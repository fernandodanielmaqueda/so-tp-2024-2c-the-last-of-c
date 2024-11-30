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
	int retval = 0, status;

	t_Socket *sockets[] = { &(CONNECTION_CPU_DISPATCH.socket_connection), &(CONNECTION_CPU_INTERRUPT.socket_connection) , NULL};
	register unsigned int i;

	for(i = 0; sockets[i] != NULL; i++) {
		if(sockets[i]->bool_thread.running) {
			if((status = pthread_cancel(sockets[i]->bool_thread.thread))) {
				log_error_pthread_cancel(status);
				retval = -1;
			}
		}
	}

	for(i = 0; sockets[i] != NULL; i++) {
		if(sockets[i]->bool_thread.running) {
			if((status = pthread_join(sockets[i]->bool_thread.thread, NULL))) {
				log_error_pthread_join(status);
				retval = -1;
			}
		}
	}

	for(i = 0; sockets[i] != NULL; i++) {
		if(sockets[i]->fd >= 0) {
			if(close(sockets[i]->fd)) {
				log_error_close();
				retval = -1;
			}
			sockets[i]->fd = -1;
		}
	}

	return retval;
}