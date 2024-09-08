/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Connection TEMPORAL_CONNECTION_MEMORY;

t_list *LIST_CONNECTIONS_MEMORY;

t_Connection CONNECTION_CPU_DISPATCH;
t_Connection CONNECTION_CPU_INTERRUPT;

void initialize_sockets(void) {

	LIST_CONNECTIONS_MEMORY = list_create();

	pthread_t thread_kernel_connect_to_cpu_dispatch;
	pthread_t thread_kernel_connect_to_cpu_interrupt;

	// [Client] Kernel -> [Server] CPU (Dispatch Port)
	pthread_create(&thread_kernel_connect_to_cpu_dispatch, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_DISPATCH);
	// [Client] Kernel -> [Server] CPU (Interrupt Port)
	pthread_create(&thread_kernel_connect_to_cpu_interrupt, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_INTERRUPT);

	// Se bloquea hasta que se realicen todas las conexiones
	pthread_join(thread_kernel_connect_to_cpu_dispatch, NULL);
	pthread_join(thread_kernel_connect_to_cpu_interrupt, NULL);
}

void finish_sockets(void) {
	close(CONNECTION_CPU_DISPATCH.fd_connection);
	close(CONNECTION_CPU_INTERRUPT.fd_connection);
	// TODO: Cerrar todas las conexiones con Memoria
}