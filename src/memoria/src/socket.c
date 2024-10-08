
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_MEMORY;

t_Client *CLIENT_CPU = NULL;
pthread_mutex_t MUTEX_CLIENT_CPU;

t_Shared_List SHARED_LIST_CLIENTS_KERNEL = { .list = NULL };
t_Shared_List SHARED_LIST_CONNECTIONS_FILESYSTEM = { .list = NULL };

int initialize_sockets(void) {
    int status;

    if((status = pthread_mutex_init(&MUTEX_CLIENT_CPU, NULL))) {
        log_error_pthread_mutex_init(status);
        // TODO
    }

	// [Servidor] Memoria <- [Cliente(s)] Kernel + CPU
    server_thread_coordinator(&SERVER_MEMORY, memory_client_handler);

    return 0;
}

int finish_sockets(void) {
    int retval = 0;

	if(close(SERVER_MEMORY.fd_listen)) {
        log_error_close();
        retval = -1;
    }

    return retval;
}

void memory_client_handler(t_Client *new_client) {
    int status;

	if((status = pthread_create(&(new_client->thread_client_handler), NULL, (void *(*)(void *)) memory_thread_for_client, (void *) new_client))) {
        log_error_pthread_create(status);
        // TODO
    }

	if((status = pthread_detach(new_client->thread_client_handler))) {
        log_error_pthread_detach(status);
        // TODO
    }
}

void *memory_thread_for_client(t_Client *new_client) {

    log_trace(MODULE_LOGGER, "[%d] Manejador de [Cliente] %s iniciado", new_client->fd_client, PORT_NAMES[new_client->client_type]);

    e_Port_Type port_type;
    int status;

    if(receive_port_type(&port_type, new_client->fd_client)) {
        log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
        if(close(new_client->fd_client)) {
            log_error_close();
        }
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

            if((status = pthread_mutex_lock(&(SHARED_LIST_CLIENTS_KERNEL.mutex)))) {
                log_error_pthread_mutex_lock(status);
                // TODO
            }
                list_add(SHARED_LIST_CLIENTS_KERNEL.list, new_client);
            if((status = pthread_mutex_unlock(&(SHARED_LIST_CLIENTS_KERNEL.mutex)))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }

            listen_kernel(new_client->fd_client);
            break;

        case CPU_PORT_TYPE:
            new_client->client_type = CPU_PORT_TYPE;

            if((status = pthread_mutex_lock(&MUTEX_CLIENT_CPU)) ){
                log_error_pthread_mutex_lock(status);
                // TODO
            }

                if(CLIENT_CPU != NULL) {
                    if((status = pthread_mutex_unlock(&MUTEX_CLIENT_CPU))) {
                        log_error_pthread_mutex_unlock(status);
                        // TODO
                    }
                    log_warning(SOCKET_LOGGER, "[%d] Ya conectado un [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                    send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);
                    break;
                }

                if(send_port_type(MEMORY_PORT_TYPE, new_client->fd_client)) {
                    if((status = pthread_mutex_unlock(&MUTEX_CLIENT_CPU))) {
                        log_error_pthread_mutex_unlock(status);
                        // TODO
                    }
                    log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                    break;
                }

                log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

                CLIENT_CPU = new_client;

            if((status = pthread_mutex_unlock(&MUTEX_CLIENT_CPU))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }

            listen_cpu();
            break;

        default:
            log_warning(SOCKET_LOGGER, "[%d] No reconocido Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
            send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);
            break;
    }

    if(close(new_client->fd_client)) {
        log_error_close();
    }
    free(new_client);
    return NULL;
}