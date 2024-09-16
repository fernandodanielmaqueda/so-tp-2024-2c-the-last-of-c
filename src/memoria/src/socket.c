
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Connection TEMPORAL_CONNECTION_FILESYSTEM;

t_Server SERVER_MEMORY;

t_Client *CLIENT_CPU = NULL;
pthread_mutex_t MUTEX_CLIENT_CPU;

t_Shared_List SHARED_LIST_CLIENTS_KERNEL;
t_Shared_List SHARED_LIST_CONNECTIONS_FILESYSTEM;

void initialize_sockets(void) {

    pthread_mutex_init(&MUTEX_CLIENT_CPU, NULL);

	// [Servidor] Memoria <- [Cliente(s)] Kernel + CPU
    memory_start_server(&SERVER_MEMORY);
}

void finish_sockets(void) {
	close(SERVER_MEMORY.fd_listen);
}

void *memory_start_server(t_Server *server) {

    log_trace(MODULE_LOGGER, "Hilo coordinador de [Servidor] %s iniciado", PORT_NAMES[server->server_type]);

	int fd_new_client;
	t_Client *new_client;

	server_start(server);

	while(1) {
		log_trace(SOCKET_LOGGER, "Esperando [Cliente] %s en Puerto: %s", PORT_NAMES[server->clients_type], server->port);
		fd_new_client = server_accept(server->fd_listen);

		if(fd_new_client == -1) {
			log_warning(SOCKET_LOGGER, "Fallo al aceptar [Cliente] %s en Puerto: %s", PORT_NAMES[server->clients_type], server->port);
			continue;
		}

        new_client = malloc(sizeof(t_Client));
		if(new_client == NULL) {
			log_warning(SOCKET_LOGGER, "malloc: No se pudieron reservar %zu bytes para [Cliente] %s en Puerto: %s", sizeof(t_Client), PORT_NAMES[server->clients_type], server->port);
			close(fd_new_client);
            continue;
		}

		log_trace(SOCKET_LOGGER, "Aceptado [Cliente] %s en Puerto: %s", PORT_NAMES[server->clients_type], server->port);
		new_client->fd_client = fd_new_client;
        new_client->client_type = server->clients_type;
        new_client->server = server;
		pthread_create(&(new_client->thread_client_handler), NULL, (void *(*)(void *)) memory_client_handler, (void *) new_client);
		pthread_detach(new_client->thread_client_handler);
	}

	return NULL;
}

void *memory_client_handler(t_Client *new_client) {

    log_trace(MODULE_LOGGER, "Hilo manejador de [Cliente] %s iniciado", PORT_NAMES[new_client->client_type]);

    e_Port_Type port_type;

    if(receive_port_type(&port_type, new_client->fd_client)) {
        log_warning(SOCKET_LOGGER, "Error al recibir Handshake de [Cliente]");

        close(new_client->fd_client);
        free(new_client);

        return NULL;
    }

    switch(port_type) {
        case KERNEL_PORT_TYPE:
            new_client->client_type = KERNEL_PORT_TYPE;

            if(send_port_type(MEMORY_PORT_TYPE, new_client->fd_client)) {
                log_warning(SOCKET_LOGGER, "Error al enviar Handshake a [Cliente] Kernel");

                close(new_client->fd_client);
                free(new_client);

                return NULL;
            }

            log_debug(SOCKET_LOGGER, "OK Handshake con [Cliente] Kernel");

            pthread_mutex_lock(&(SHARED_LIST_CLIENTS_KERNEL.mutex));
                list_add(SHARED_LIST_CLIENTS_KERNEL.list, new_client);
            pthread_mutex_unlock(&(SHARED_LIST_CLIENTS_KERNEL.mutex));

            listen_kernel(new_client);

            return NULL;

        case CPU_PORT_TYPE:
            new_client->client_type = CPU_PORT_TYPE;

            pthread_mutex_lock(&MUTEX_CLIENT_CPU);

                if(CLIENT_CPU != NULL) {
                    pthread_mutex_unlock(&MUTEX_CLIENT_CPU);
                    log_warning(SOCKET_LOGGER, "Ya conectado un [Cliente] CPU");
                    send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);

                    close(new_client->fd_client);
                    free(new_client);

                    return NULL;
                }

                if(send_port_type(MEMORY_PORT_TYPE, new_client->fd_client)) {
                    pthread_mutex_unlock(&MUTEX_CLIENT_CPU);
                    log_warning(SOCKET_LOGGER, "Error al enviar Handshake a [Cliente] CPU");

                    close(new_client->fd_client);
                    free(new_client);

                    return NULL;
                }

                log_debug(SOCKET_LOGGER, "OK Handshake con [Cliente] CPU");

                CLIENT_CPU = new_client;

            pthread_mutex_unlock(&MUTEX_CLIENT_CPU);

            listen_cpu();

            return NULL;

        default:
            log_warning(SOCKET_LOGGER, "No reconocido Handshake de [Cliente]");
            send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);

            close(new_client->fd_client);
            free(new_client);

            return NULL;
    }

    return NULL;
}

bool client_matches_pthread(t_Client *client, pthread_t *thread) {
    return pthread_equal(client->thread_client_handler, *thread);
}