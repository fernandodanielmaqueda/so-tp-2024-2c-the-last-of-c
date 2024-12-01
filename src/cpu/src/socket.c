/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_CPU_DISPATCH;
t_Server SERVER_CPU_INTERRUPT;

t_Client CLIENT_KERNEL_CPU_DISPATCH;
t_Client CLIENT_KERNEL_CPU_INTERRUPT;

t_Connection CONNECTION_MEMORY;

void initialize_sockets(void) {
    int status;

    // [Server] CPU (Dispatch) <- [Cliente] Kernel
    if((status = pthread_create(&(SERVER_CPU_DISPATCH.socket_listen.bool_thread.thread), NULL, (void *(*)(void *)) server_thread_for_client, (void *) &CLIENT_KERNEL_CPU_DISPATCH))) {
        log_error_pthread_create(status);
        exit_sigint();
    }
    SERVER_CPU_DISPATCH.socket_listen.bool_thread.running = true;

    // [Server] CPU (Interrupt) <- [Cliente] Kernel
    if((status = pthread_create(&(SERVER_CPU_INTERRUPT.socket_listen.bool_thread.thread), NULL, (void *(*)(void *)) server_thread_for_client, (void *) &CLIENT_KERNEL_CPU_INTERRUPT))) {
        log_error_pthread_create(status);
        exit_sigint();
    }
    SERVER_CPU_INTERRUPT.socket_listen.bool_thread.running = true;

    // [Client] CPU -> [Server] Memoria
    if((status = pthread_create(&(CONNECTION_MEMORY.socket_connection.bool_thread.thread), NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_MEMORY))) {
        log_error_pthread_create(status);
        exit_sigint();
    }
    CONNECTION_MEMORY.socket_connection.bool_thread.running = true;

    // Se bloquea hasta que se realicen todas las conexiones
    if((status = pthread_join(SERVER_CPU_DISPATCH.socket_listen.bool_thread.thread, NULL))) {
        log_error_pthread_join(status);
        exit_sigint();
    }
    SERVER_CPU_DISPATCH.socket_listen.bool_thread.running = false;

    if((status = pthread_join(SERVER_CPU_INTERRUPT.socket_listen.bool_thread.thread, NULL))) {
        log_error_pthread_join(status);
        exit_sigint();
    }
    SERVER_CPU_INTERRUPT.socket_listen.bool_thread.running = false;

    if((status = pthread_join(CONNECTION_MEMORY.socket_connection.bool_thread.thread, NULL))) {
        log_error_pthread_join(status);
        exit_sigint();
    }
    CONNECTION_MEMORY.socket_connection.bool_thread.running = false;
}

int finish_sockets(void) {
	t_Socket *sockets[] = { &(SERVER_CPU_DISPATCH.socket_listen), &(CLIENT_KERNEL_CPU_DISPATCH.socket_client), &(SERVER_CPU_INTERRUPT.socket_listen), &(CLIENT_KERNEL_CPU_INTERRUPT.socket_client), &(CONNECTION_MEMORY.socket_connection), NULL};
	if(socket_array_finish(sockets)) {
		return -1;
	}

    return 0;
}