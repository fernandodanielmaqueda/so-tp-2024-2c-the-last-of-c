/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_FILESYSTEM;

t_Shared_List SHARED_LIST_CLIENTS_MEMORY;

void initialize_sockets(void) {

	int fd_new_client;
	t_Client *new_client;

	server_start(&SERVER_FILESYSTEM);

	while(1) {

		log_trace(SOCKET_LOGGER, "Esperando [Cliente] %s en Puerto: %s", PORT_NAMES[SERVER_FILESYSTEM.clients_type], SERVER_FILESYSTEM.port);
		fd_new_client = server_accept(SERVER_FILESYSTEM.fd_listen);

		if(fd_new_client == -1) {
			log_warning(SOCKET_LOGGER, "Fallo al aceptar [Cliente] %s en Puerto: %s", PORT_NAMES[SERVER_FILESYSTEM.clients_type], SERVER_FILESYSTEM.port);
			continue;
		}

		new_client = malloc(sizeof(t_Client));
		if(new_client == NULL) {
			log_error(SOCKET_LOGGER, "Error al reservar memoria para [Cliente] %s en Puerto: %s", PORT_NAMES[SERVER_FILESYSTEM.clients_type], SERVER_FILESYSTEM.port);
			exit(1);
		}

		log_trace(SOCKET_LOGGER, "Aceptado [Cliente] %s en Puerto: %s", PORT_NAMES[SERVER_FILESYSTEM.clients_type], SERVER_FILESYSTEM.port);
		new_client->fd_client = fd_new_client;
		new_client->client_type = SERVER_FILESYSTEM.clients_type;
		new_client->server = &SERVER_FILESYSTEM;
		pthread_create(&(new_client->thread_client_handler), NULL, (void *(*)(void *)) filesystem_client_handler_for_memory, (void *) new_client);
		pthread_detach(new_client->thread_client_handler);
	}

	return;

}

void finish_sockets(void) {
	//close(CONNECTION_KERNEL.fd_connection);
	//close(CONNECTION_MEMORY.fd_connection);
}