/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_FILESYSTEM;

t_Shared_List SHARED_LIST_CLIENTS_MEMORY;

void initialize_sockets(void) {

	// [Servidor] Filesystem <- [Cliente(s)] Memoria
	filesystem_start_server(&SERVER_FILESYSTEM);

}

void finish_sockets(void) {
	//close(CONNECTION_KERNEL.fd_connection);
	//close(CONNECTION_MEMORY.fd_connection);
}

void *filesystem_start_server(t_Server *server) {

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
		pthread_create(&(new_client->thread_client_handler), NULL, (void *(*)(void *)) filesystem_client_handler_for_memory, (void *) new_client);
		pthread_detach(new_client->thread_client_handler);
	}

	return NULL;
}