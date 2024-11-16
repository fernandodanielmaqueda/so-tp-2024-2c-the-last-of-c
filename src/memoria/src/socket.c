
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_MEMORY;

t_Client *CLIENT_CPU = NULL;
pthread_mutex_t MUTEX_CLIENT_CPU;

t_Shared_List SHARED_LIST_CLIENTS = { .list = NULL };
pthread_cond_t COND_CLIENTS;

void memory_client_handler(t_Client *new_client) {
    int status;

    pthread_cleanup_push((void (*)(void *)) free, (void *) new_client);
    pthread_cleanup_push((void (*)(void *)) wrapper_close, (void *) &(new_client->fd_client));

        new_client->thread_client_handler.running = false;

        bool join = false;
        t_Bool_Join_Thread join_thread = { .thread = &(new_client->thread_client_handler.thread), .join = &join };
        pthread_cleanup_push((void (*)(void *)) wrapper_pthread_join, &join_thread);

            if((status = pthread_mutex_lock(&(SHARED_LIST_CLIENTS.mutex)))) {
                log_error_pthread_mutex_lock(status);
                error_pthread();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_CLIENTS.mutex));

                if((status = pthread_create(&(new_client->thread_client_handler.thread), NULL, (void *(*)(void *)) memory_thread_for_client, (void *) new_client))) {
                    log_error_pthread_create(status);
                    error_pthread();
                }
                pthread_cleanup_push((void (*)(void *)) wrapper_pthread_cancel, &(new_client->thread_client_handler.thread));

                    join = true;
                        if((status = pthread_detach(new_client->thread_client_handler.thread))) {
                            log_error_pthread_detach(status);
                            error_pthread();
                        }
                    join = false;

                pthread_cleanup_pop(0); // cancel_thread

                new_client->thread_client_handler.running = true;

                list_add((SHARED_LIST_CLIENTS.list), new_client);

            pthread_cleanup_pop(0);
            if((status = pthread_mutex_unlock(&(SHARED_LIST_CLIENTS.mutex)))) {
                join = false;
                log_error_pthread_mutex_unlock(status);
                error_pthread();
            }

        pthread_cleanup_pop(0); // join_thread

    pthread_cleanup_pop(0); // fd_client
    pthread_cleanup_pop(0); // new_client

}

void *memory_thread_for_client(t_Client *new_client) {

    pthread_t thread = pthread_self();
    int result = 0, status;

	pthread_cleanup_push((void (*)(void *)) remove_client_thread, &thread);

    log_trace(MODULE_LOGGER, "[%d] Manejador de [Cliente] %s iniciado", new_client->fd_client, PORT_NAMES[new_client->client_type]);

    e_Port_Type port_type;

    if(receive_port_type(&port_type, new_client->fd_client)) {
        log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
        error_pthread();
    }
    log_trace(SOCKET_LOGGER, "[%d] Se recibe Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[port_type]);

    switch(port_type) {
        case KERNEL_PORT_TYPE:
        {
            new_client->client_type = KERNEL_PORT_TYPE;

            if(send_port_type(MEMORY_PORT_TYPE, new_client->fd_client)) {
                log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                error_pthread();
            }
            log_trace(SOCKET_LOGGER, "[%d] Se envia Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

            log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

            listen_kernel(new_client->fd_client);
            break;
        }

        case CPU_PORT_TYPE:
        {
            new_client->client_type = CPU_PORT_TYPE;

            if((status = pthread_mutex_lock(&MUTEX_CLIENT_CPU))) {
                log_error_pthread_mutex_lock(status);
                error_pthread();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_CLIENT_CPU);

                if(CLIENT_CPU != NULL) {
                    log_warning(SOCKET_LOGGER, "[%d] Ya conectado un [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                    result = -1;
                    goto cleanup_mutex_client_cpu;
                }

                CLIENT_CPU = new_client;

            cleanup_mutex_client_cpu:
            pthread_cleanup_pop(0); // MUTEX_CLIENT_CPU
            if((status = pthread_mutex_unlock(&MUTEX_CLIENT_CPU))) {
                log_error_pthread_mutex_unlock(status);
                error_pthread();
            }

            if(result) {
                if(send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client)) {
                    log_warning(SOCKET_LOGGER, "[%d] Error al enviar no reconocimiento de Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                    error_pthread();
                }
                log_trace(SOCKET_LOGGER, "[%d] Se envia no reconocimiento de Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

                break;
            }

            if(send_port_type(MEMORY_PORT_TYPE, new_client->fd_client)) {
                log_warning(SOCKET_LOGGER, "[%d] Error al enviar reconocimiento de Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);
                error_pthread();
            }
            log_trace(SOCKET_LOGGER, "[%d] Se envia reconocimiento de Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

            log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->client_type]);

            listen_cpu();
            break;
        }

        default:
        {
            log_warning(SOCKET_LOGGER, "[%d] No se reconoce Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[port_type]);

            if(send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client)) {
                log_warning(SOCKET_LOGGER, "[%d] Error al enviar no reconocimiento de Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[port_type]);
                error_pthread();
            }
            log_trace(SOCKET_LOGGER, "[%d] Se envia no reconocimiento de Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[port_type]);

            break;
        }
    }

    pthread_cleanup_pop(0); // new_client
    if(remove_client_thread(&thread)) {
        error_pthread();
    }

    return NULL;
}

int remove_client_thread(pthread_t *thread) {
    //pthread_cleanup_push((void (*)(void *)) free, (void *) new_client);
    //pthread_cleanup_push((void (*)(void *)) wrapper_close, (void *) &(new_client->fd_client));
    return 0;
}