/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Connection CONNECTION_CPU_DISPATCH;
t_Connection CONNECTION_CPU_INTERRUPT;

void initialize_sockets(void) {
	int status;

	// [Client] Kernel -> [Server] CPU (Dispatch Port)
	if((status = pthread_create(&(CONNECTION_CPU_DISPATCH.socket_connection.bool_thread.thread), NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_DISPATCH))) {
		log_error_pthread_create(status);
		exit_sigint();
	}
	CONNECTION_CPU_DISPATCH.socket_connection.bool_thread.running = true;

	// [Client] Kernel -> [Server] CPU (Interrupt Port)
	if((status = pthread_create(&(CONNECTION_CPU_INTERRUPT.socket_connection.bool_thread.thread), NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_INTERRUPT))) {
		log_error_pthread_create(status);
		exit_sigint();
	}
	CONNECTION_CPU_INTERRUPT.socket_connection.bool_thread.running = true;

	// Se bloquea hasta que se realicen todas las conexiones
	if((status = pthread_join(CONNECTION_CPU_INTERRUPT.socket_connection.bool_thread.thread, NULL))) {
		log_error_pthread_join(status);
		exit_sigint();
	}
	CONNECTION_CPU_DISPATCH.socket_connection.bool_thread.running = false;

	if((status = pthread_join(CONNECTION_CPU_DISPATCH.socket_connection.bool_thread.thread, NULL))) {
		log_error_pthread_join(status);
		exit_sigint();
	}
	CONNECTION_CPU_INTERRUPT.socket_connection.bool_thread.running = false;
}

int finish_sockets(void) {
	t_Socket *sockets[] = { &(CONNECTION_CPU_DISPATCH.socket_connection), &(CONNECTION_CPU_INTERRUPT.socket_connection) , NULL};
	if(socket_array_finish(sockets)) {
		return -1;
	}

	return 0;
}