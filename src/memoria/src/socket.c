
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

	bool created = false;
	t_Conditional_Cleanup new_client_cleanup = { .function = (void (*)(void *)) free, .argument = (void *) new_client, .condition = &created, .negate_condition = true };
    pthread_cleanup_push((void (*)(void *)) conditional_cleanup, (void *) &new_client_cleanup);
    t_Conditional_Cleanup fd_client_cleanup = { .function = (void (*)(void *)) wrapper_close, .argument = (void *) &(new_client->socket_client.fd), .condition = &created, .negate_condition = true };
    pthread_cleanup_push((void (*)(void *)) conditional_cleanup, (void *) &fd_client_cleanup);

        new_client->socket_client.bool_thread.running = false;

        bool join = false;
        t_Conditional_Cleanup join_cleanup = { .function = (void (*)(void *)) wrapper_pthread_join, .argument = (void *) &(new_client->socket_client.bool_thread.thread), .condition = &join, .negate_condition = false };
        pthread_cleanup_push((void (*)(void *)) conditional_cleanup, (void *) &join_cleanup);

            if((status = pthread_mutex_lock(&(SHARED_LIST_CLIENTS.mutex)))) {
                report_error_pthread_mutex_lock(status);
                exit_sigint();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_CLIENTS.mutex));

                if((status = pthread_create(&(new_client->socket_client.bool_thread.thread), NULL, (void *(*)(void *)) memory_thread_for_client, (void *) new_client))) {
                    report_error_pthread_create(status);
                    exit_sigint();
                }
                pthread_cleanup_push((void (*)(void *)) wrapper_pthread_cancel, &(new_client->socket_client.bool_thread.thread));

                    created = true;

                    join = true;
                        if((status = pthread_detach(new_client->socket_client.bool_thread.thread))) {
                            report_error_pthread_detach(status);
                            exit_sigint();
                        }
                    join = false;

                pthread_cleanup_pop(0); // cancel_thread

                new_client->socket_client.bool_thread.running = true;

                list_add((SHARED_LIST_CLIENTS.list), new_client);

            pthread_cleanup_pop(0);
            if((status = pthread_mutex_unlock(&(SHARED_LIST_CLIENTS.mutex)))) {
                join = false;
                report_error_pthread_mutex_unlock(status);
                exit_sigint();
            }

        pthread_cleanup_pop(0); // join_thread

    pthread_cleanup_pop(0); // fd_client
    pthread_cleanup_pop(0); // new_client

}

void *memory_thread_for_client(t_Client *new_client) {

    //pthread_t thread = pthread_self();
    int result = 0, status;

	pthread_cleanup_push((void (*)(void *)) remove_client_thread, new_client);

    log_trace(MODULE_LOGGER, "[%d] Manejador de [Cliente] %s iniciado", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);

    e_Port_Type port_type;

    if(receive_port_type(&port_type, new_client->socket_client.fd)) {
        log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
        exit_sigint();
    }
    log_trace(SOCKET_LOGGER, "[%d] Se recibe Handshake de [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[port_type]);

    switch(port_type) {
        case KERNEL_PORT_TYPE:
        {
            new_client->client_type = KERNEL_PORT_TYPE;

            if(send_port_type(MEMORY_PORT_TYPE, new_client->socket_client.fd)) {
                log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);
                exit_sigint();
            }
            log_trace(SOCKET_LOGGER, "[%d] Se envía Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);

            log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);

            listen_kernel(new_client->socket_client.fd);
            break;
        }

        case CPU_PORT_TYPE:
        {
            new_client->client_type = CPU_PORT_TYPE;

            if((status = pthread_mutex_lock(&MUTEX_CLIENT_CPU))) {
                report_error_pthread_mutex_lock(status);
                exit_sigint();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_CLIENT_CPU);

                if(CLIENT_CPU != NULL) {
                    log_warning(SOCKET_LOGGER, "[%d] Ya conectado un [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);
                    result = -1;
                    goto cleanup_mutex_client_cpu;
                }

                CLIENT_CPU = new_client;

            cleanup_mutex_client_cpu:
            pthread_cleanup_pop(0); // MUTEX_CLIENT_CPU
            if((status = pthread_mutex_unlock(&MUTEX_CLIENT_CPU))) {
                report_error_pthread_mutex_unlock(status);
                exit_sigint();
            }

            if(result) {
                if(send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->socket_client.fd)) {
                    log_warning(SOCKET_LOGGER, "[%d] Error al enviar no reconocimiento de Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);
                    exit_sigint();
                }
                log_trace(SOCKET_LOGGER, "[%d] Se envía no reconocimiento de Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);

                break;
            }

            if(send_port_type(MEMORY_PORT_TYPE, new_client->socket_client.fd)) {
                log_warning(SOCKET_LOGGER, "[%d] Error al enviar reconocimiento de Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);
                exit_sigint();
            }
            log_trace(SOCKET_LOGGER, "[%d] Se envía reconocimiento de Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);

            log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);

            listen_cpu();
            break;
        }

        default:
        {
            log_warning(SOCKET_LOGGER, "[%d] No se reconoce Handshake de [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[port_type]);

            if(send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->socket_client.fd)) {
                log_warning(SOCKET_LOGGER, "[%d] Error al enviar no reconocimiento de Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[port_type]);
                exit_sigint();
            }
            log_trace(SOCKET_LOGGER, "[%d] Se envía no reconocimiento de Handshake a [Cliente] %s", new_client->socket_client.fd, PORT_NAMES[port_type]);

            break;
        }
    }

    pthread_cleanup_pop(0); // new_client
    if(remove_client_thread(new_client)) {
        exit_sigint();
    }

    return NULL;
}

int remove_client_thread(t_Client *client) {
	int retval = 0, status;

    pthread_cleanup_push((void (*)(void *)) free, client);
    pthread_cleanup_push((void (*)(void *)) wrapper_close, &(client->socket_client.fd));

        if((status = pthread_mutex_lock(&(SHARED_LIST_CLIENTS.mutex)))) {
            report_error_pthread_mutex_lock(status);
            retval = -1;
            goto cleanup_fd_client;
        }
        pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_CLIENTS.mutex));

            list_remove_by_condition_with_comparation(SHARED_LIST_CLIENTS.list, (bool (*)(void *, void *)) pointers_match, client);

            if(signal_client_threads()) {
                retval = -1;
                goto cleanup_mutex_clients;
            }

        cleanup_mutex_clients:
        pthread_cleanup_pop(0); // SHARED_LIST_CLIENTS.mutex
        if((status = pthread_mutex_unlock(&(SHARED_LIST_CLIENTS.mutex)))) {
            report_error_pthread_mutex_unlock(status);
            retval = -1;
            goto cleanup_fd_client;
        }

    cleanup_fd_client:
    pthread_cleanup_pop(0); // fd_client
    if(close(client->socket_client.fd)) {
        report_error_close();
        retval = -1;
    }

    pthread_cleanup_pop(1); // new_client

    return retval;
}

int wait_client_threads(void) {
    int retval = 0, status;

    if((status = pthread_mutex_lock(&(SHARED_LIST_CLIENTS.mutex)))) {
        report_error_pthread_mutex_lock(status);
        return -1;
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_CLIENTS.mutex));

        t_link_element *current = SHARED_LIST_CLIENTS.list->head;
        while(current != NULL) {
            t_Client *client = current->data;
            if((status = pthread_cancel(client->socket_client.bool_thread.thread))) {
                report_error_pthread_cancel(status);
                retval = -1;
                goto cleanup_mutex_clients;
            }
            current = current->next;
        }

        //log_trace(MODULE_LOGGER, "Esperando a que finalicen los hilos de [Cliente] %s", PORT_NAMES[MEMORY_PORT_TYPE]);
        
        while(SHARED_LIST_CLIENTS.list->head != NULL) {
            if((status = pthread_cond_wait(&COND_CLIENTS, &(SHARED_LIST_CLIENTS.mutex)))) {
                report_error_pthread_cond_wait(status);
                retval = -1;
                break;
            }
        }

    cleanup_mutex_clients:
    pthread_cleanup_pop(0); // SHARED_LIST_CLIENTS.mutex
    if((status = pthread_mutex_unlock(&(SHARED_LIST_CLIENTS.mutex)))) {
        report_error_pthread_mutex_unlock(status);
        return -1;
    }

    return retval;
}

int signal_client_threads(void) {
    int status;

    if(SHARED_LIST_CLIENTS.list->head == NULL) {
        if((status = pthread_cond_signal(&COND_CLIENTS))) {
            report_error_pthread_cond_signal(status);
            return -1;
        }
    }

    return 0;
}