/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_CPU_DISPATCH;
t_Server SERVER_CPU_INTERRUPT;

t_Client CLIENT_KERNEL_CPU_DISPATCH;
t_Client CLIENT_KERNEL_CPU_INTERRUPT;

t_Connection CONNECTION_MEMORY;

int initialize_sockets(void) {
    pthread_t thread_cpu_connect_to_memory;
    int status;

    // [Server] CPU (Dispatch) <- [Cliente] Kernel
    if((status = pthread_create(&(SERVER_CPU_DISPATCH.thread_server), NULL, (void *(*)(void *)) server_thread_for_client, (void *) &CLIENT_KERNEL_CPU_DISPATCH))) {
        log_error_pthread_create(status);
        // TODO
    }

    // [Server] CPU (Interrupt) <- [Cliente] Kernel
    if((status = pthread_create(&(SERVER_CPU_INTERRUPT.thread_server), NULL, (void *(*)(void *)) server_thread_for_client, (void *) &CLIENT_KERNEL_CPU_INTERRUPT))) {
        log_error_pthread_create(status);
        // TODO
    }

    // [Client] CPU -> [Server] Memoria
    if((status = pthread_create(&thread_cpu_connect_to_memory, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_MEMORY))) {
        log_error_pthread_create(status);
        // TODO
    }

    // Se bloquea hasta que se realicen todas las conexiones
    if((status = pthread_join(SERVER_CPU_DISPATCH.thread_server, NULL))) {
        log_error_pthread_join(status);
        // TODO
    }

    if((status = pthread_join(SERVER_CPU_INTERRUPT.thread_server, NULL))) {
        log_error_pthread_join(status);
        // TODO
    }

    if((status = pthread_join(thread_cpu_connect_to_memory, NULL))) {
        log_error_pthread_join(status);
        // TODO
    }

    return 0;
}

void finish_sockets(void) {
    close(CLIENT_KERNEL_CPU_DISPATCH.fd_client);
    close(CLIENT_KERNEL_CPU_INTERRUPT.fd_client);
    close(CONNECTION_MEMORY.fd_connection);
}