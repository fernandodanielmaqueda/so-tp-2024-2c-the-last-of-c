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

    // [Server] CPU (Dispatch) <- [Cliente] Kernel
    pthread_create(&(SERVER_CPU_DISPATCH.thread_server), NULL, (void *(*)(void *)) server_thread_for_client, (void *) &CLIENT_KERNEL_CPU_DISPATCH);
    // [Server] CPU (Interrupt) <- [Cliente] Kernel
    pthread_create(&(SERVER_CPU_INTERRUPT.thread_server), NULL, (void *(*)(void *)) server_thread_for_client, (void *) &CLIENT_KERNEL_CPU_INTERRUPT);
    // [Client] CPU -> [Server] Memoria
    pthread_create(&thread_cpu_connect_to_memory, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_MEMORY);

    // Se bloquea hasta que se realicen todas las conexiones
    pthread_join(SERVER_CPU_DISPATCH.thread_server, NULL);
    pthread_join(SERVER_CPU_INTERRUPT.thread_server, NULL);
    pthread_join(thread_cpu_connect_to_memory, NULL);

    return 0;
}

void finish_sockets(void) {
    close(CLIENT_KERNEL_CPU_DISPATCH.fd_client);
    close(CLIENT_KERNEL_CPU_INTERRUPT.fd_client);
    close(CONNECTION_MEMORY.fd_connection);
}