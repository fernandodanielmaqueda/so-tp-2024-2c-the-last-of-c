/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Connection CONNECTION_CPU_DISPATCH;
t_Connection CONNECTION_CPU_INTERRUPT;

void initialize_sockets(void) {
	int status;

	// [Client] Kernel -> [Server] CPU (Dispatch Port)
	if((status = pthread_create(&CONNECTION_CPU_DISPATCH.thread_connection.thread, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_DISPATCH))) {
		log_error_pthread_create(status);
		exit_sigint();
	}
	CONNECTION_CPU_DISPATCH.thread_connection.running = true;

	// [Client] Kernel -> [Server] CPU (Interrupt Port)
	if((status = pthread_create(&CONNECTION_CPU_INTERRUPT.thread_connection.thread, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_INTERRUPT))) {
		log_error_pthread_create(status);
		exit_sigint();
	}
	CONNECTION_CPU_INTERRUPT.thread_connection.running = true;

	// Se bloquea hasta que se realicen todas las conexiones
	if((status = pthread_join(CONNECTION_CPU_INTERRUPT.thread_connection.thread, NULL))) {
		log_error_pthread_join(status);
		exit_sigint();
	}
	CONNECTION_CPU_DISPATCH.thread_connection.running = false;

	if((status = pthread_join(CONNECTION_CPU_DISPATCH.thread_connection.thread, NULL))) {
		log_error_pthread_join(status);
		exit_sigint();
	}
	CONNECTION_CPU_INTERRUPT.thread_connection.running = false;

}

int finish_sockets(void) {
	int retval = 0, status;

	t_Connection *connections[] = { &CONNECTION_CPU_DISPATCH, &CONNECTION_CPU_INTERRUPT , NULL};
	register unsigned int i;

	for(i = 0; connections[i] != NULL; i++) {
		if(connections[i]->thread_connection.running) {
			if((status = pthread_cancel(connections[i]->thread_connection.thread))) {
				log_error_pthread_cancel(status);
				retval = -1;
			}
		}
	}

	for(i = 0; connections[i] != NULL; i++) {
		if(connections[i]->thread_connection.running) {
			if((status = pthread_join(connections[i]->thread_connection.thread, NULL))) {
				log_error_pthread_join(status);
				retval = -1;
			}
		}
	}

	for(i = 0; connections[i] != NULL; i++) {
		if(connections[i]->fd_connection >= 0) {
			if(close(connections[i]->fd_connection)) {
				log_error_close();
				retval = -1;
			}
			connections[i]->fd_connection = -1;
		}
	}

	return retval;
}