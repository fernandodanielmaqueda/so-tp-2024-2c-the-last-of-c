/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server COORDINATOR_IO;
t_Connection CONNECTION_MEMORY;
t_Connection CONNECTION_CPU_DISPATCH;
t_Connection CONNECTION_CPU_INTERRUPT;

void initialize_sockets(void) {

	pthread_t thread_kernel_connect_to_memory;
	pthread_t thread_kernel_connect_to_cpu_dispatch;
	pthread_t thread_kernel_connect_to_cpu_interrupt;

	// [Server] Kernel <- [Cliente(s)] Entrada/Salida
	pthread_create(&(COORDINATOR_IO.thread_server), NULL, (void *(*)(void *)) kernel_start_server_for_io, (void *) &COORDINATOR_IO);
	// [Client] Kernel -> [Server] Memoria
	pthread_create(&thread_kernel_connect_to_memory, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_MEMORY);
	// [Client] Kernel -> [Server] CPU (Dispatch Port)
	pthread_create(&thread_kernel_connect_to_cpu_dispatch, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_DISPATCH);
	// [Client] Kernel -> [Server] CPU (Interrupt Port)
	pthread_create(&thread_kernel_connect_to_cpu_interrupt, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_CPU_INTERRUPT);

	// Se bloquea hasta que se realicen todas las conexiones
	pthread_join(thread_kernel_connect_to_memory, NULL);
	pthread_join(thread_kernel_connect_to_cpu_dispatch, NULL);
	pthread_join(thread_kernel_connect_to_cpu_interrupt, NULL);
}

void finish_sockets(void) {
	close(COORDINATOR_IO.fd_listen);
	close(CONNECTION_MEMORY.fd_connection);
	close(CONNECTION_CPU_DISPATCH.fd_connection);
	close(CONNECTION_CPU_INTERRUPT.fd_connection);
	// TODO: CERRAR LOS IOs
}

void *kernel_start_server_for_io(t_Server *server) {

	log_trace(MODULE_LOGGER, "Hilo coordinador de [Cliente(s)] %s iniciado", PORT_NAMES[server->clients_type]);

	int fd_new_client;
	t_Client *new_client;

	server_start(server);

	while(1) {

		log_trace(SOCKET_LOGGER, "Esperando [Cliente] %s en Puerto: %s", PORT_NAMES[server->clients_type], server->port);
		fd_new_client = server_accept(server->fd_listen);

		if(fd_new_client == -1) {
			log_warning(SOCKET_LOGGER, "Fallo al aceptar [Cliente] %s en Puerto: %s", PORT_NAMES[server->clients_type], server->port);
			continue;
		}

		new_client = malloc(sizeof(t_Client));
		if(new_client == NULL) {
			log_error(SOCKET_LOGGER, "Error al reservar memoria para [Cliente] %s en Puerto: %s", PORT_NAMES[server->clients_type], server->port);
			exit(1);
		}

		log_trace(SOCKET_LOGGER, "Aceptado [Cliente] %s en Puerto: %s", PORT_NAMES[server->clients_type], server->port);
		new_client->fd_client = fd_new_client;
		new_client->client_type = server->clients_type;
		new_client->server = server;
		pthread_create(&(new_client->thread_client_handler), NULL, (void *(*)(void *)) kernel_client_handler_for_io, (void *) new_client);
		pthread_detach(new_client->thread_client_handler);
	}

	return NULL;
}