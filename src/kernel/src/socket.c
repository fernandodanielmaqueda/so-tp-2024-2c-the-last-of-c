/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Shared_List SHARED_LIST_CONNECTIONS_MEMORY = { .list = NULL };

t_Connection CONNECTION_CPU_DISPATCH;
t_Connection CONNECTION_CPU_INTERRUPT;

void initialize_sockets(void) {
	int status;

	pthread_t thread_kernel_connect_to_cpu_dispatch;
	// [Client] Kernel -> [Server] CPU (Dispatch Port)
	if((status = pthread_create(&thread_kernel_connect_to_cpu_dispatch, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_DISPATCH))) {
		log_error_pthread_create(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &thread_kernel_connect_to_cpu_dispatch);

	pthread_t thread_kernel_connect_to_cpu_interrupt;
	// [Client] Kernel -> [Server] CPU (Interrupt Port)
	if((status = pthread_create(&thread_kernel_connect_to_cpu_interrupt, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_INTERRUPT))) {
		log_error_pthread_create(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &thread_kernel_connect_to_cpu_interrupt);

	// Se bloquea hasta que se realicen todas las conexiones
	if((status = pthread_join(thread_kernel_connect_to_cpu_interrupt, NULL))) {
		log_error_pthread_join(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_pop(0); // thread_kernel_connect_to_cpu_interrupt

	if((status = pthread_join(thread_kernel_connect_to_cpu_dispatch, NULL))) {
		log_error_pthread_join(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_pop(0); // thread_kernel_connect_to_cpu_dispatch
}