/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Shared_List SHARED_LIST_CONNECTIONS_MEMORY = { .list = NULL };

t_Connection CONNECTION_CPU_DISPATCH;
t_Connection CONNECTION_CPU_INTERRUPT;

int initialize_sockets(void) {
	int status;

	SHARED_LIST_CONNECTIONS_MEMORY.list = list_create();
	if((status = pthread_mutex_init(&(SHARED_LIST_CONNECTIONS_MEMORY.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		// TODO
	}

	pthread_t thread_kernel_connect_to_cpu_dispatch;
	pthread_t thread_kernel_connect_to_cpu_interrupt;

	// [Client] Kernel -> [Server] CPU (Dispatch Port)
	if((status = pthread_create(&thread_kernel_connect_to_cpu_dispatch, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_DISPATCH))) {
		log_error_pthread_create(status);
		// TODO
	}
	// [Client] Kernel -> [Server] CPU (Interrupt Port)
	if((status = pthread_create(&thread_kernel_connect_to_cpu_interrupt, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_INTERRUPT))) {
		log_error_pthread_create(status);
		// TODO
	}

	// Se bloquea hasta que se realicen todas las conexiones
	if((status = pthread_join(thread_kernel_connect_to_cpu_dispatch, NULL))) {
		log_error_pthread_join(status);
		// TODO
	}

	if((status = pthread_join(thread_kernel_connect_to_cpu_interrupt, NULL))) {
		log_error_pthread_join(status);
		// TODO
	}

	return 0;
}

int finish_sockets(void) {
	int retval = 0;

	if(close(CONNECTION_CPU_DISPATCH.fd_connection)) {
		log_error_close();
		retval = -1;
	}
	if(close(CONNECTION_CPU_INTERRUPT.fd_connection)) {
		log_error_close();
		retval = -1;
	}
	// TODO: Cerrar todas las conexiones con Memoria

	return retval;
}