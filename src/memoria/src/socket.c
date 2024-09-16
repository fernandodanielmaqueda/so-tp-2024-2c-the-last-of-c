
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
    server_thread_coordinator(&SERVER_MEMORY, memory_client_handler);
}

void finish_sockets(void) {
	close(SERVER_MEMORY.fd_listen);
}

void memory_client_handler(t_Client *new_client) {
	pthread_create(&(new_client->thread_client_handler), NULL, (void *(*)(void *)) memory_thread_for_client, (void *) new_client);
	pthread_detach(new_client->thread_client_handler);
}

void *memory_thread_for_client(t_Client *new_client) {

    log_trace(MODULE_LOGGER, "[%d] Manejador de [Cliente] %s iniciado", new_client->fd_client, PORT_NAMES[new_client->client_type]);

    e_Port_Type port_type;

    if(receive_port_type(&port_type, new_client->fd_client)) {
        log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
        close(new_client->fd_client);
        free(new_client);
        return NULL;
    }

    switch(port_type) {
        case KERNEL_PORT_TYPE:
            new_client->client_type = KERNEL_PORT_TYPE;

            if(send_port_type(MEMORY_PORT_TYPE, new_client->fd_client)) {
                log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                break;
            }

            log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

            pthread_mutex_lock(&(SHARED_LIST_CLIENTS_KERNEL.mutex));
                list_add(SHARED_LIST_CLIENTS_KERNEL.list, new_client);
            pthread_mutex_unlock(&(SHARED_LIST_CLIENTS_KERNEL.mutex));

            listen_kernel(new_client->fd_client);
            break;

        case CPU_PORT_TYPE:
            new_client->client_type = CPU_PORT_TYPE;

            pthread_mutex_lock(&MUTEX_CLIENT_CPU);

                if(CLIENT_CPU != NULL) {
                    pthread_mutex_unlock(&MUTEX_CLIENT_CPU);
                    log_warning(SOCKET_LOGGER, "[%d] Ya conectado un [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                    send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);
                    break;
                }

                if(send_port_type(MEMORY_PORT_TYPE, new_client->fd_client)) {
                    pthread_mutex_unlock(&MUTEX_CLIENT_CPU);
                    log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                    break;
                }

                log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

                CLIENT_CPU = new_client;

            pthread_mutex_unlock(&MUTEX_CLIENT_CPU);

            listen_cpu();
            break;

        default:
            log_warning(SOCKET_LOGGER, "[%d] No reconocido Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
            send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);
            break;
    }

    close(new_client->fd_client);
    free(new_client);
    return NULL;
}