/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_FILESYSTEM;

t_Shared_List SHARED_LIST_CLIENTS_MEMORY = { .list = NULL };

int initialize_sockets(void) {

	// [Servidor] Filesystem <- [Cliente(s)] Memoria
	server_thread_coordinator(&SERVER_FILESYSTEM, filesystem_client_handler);

	return 0;
}

int finish_sockets(void) {
	int retval = 0;

	/*
	if(close(CONNECTION_KERNEL.fd_connection)) {
		report_error_close();
		retval = -1;
	}
	if(close(CONNECTION_MEMORY.fd_connection)) {
		report_error_close();
		retval = -1;
	}
	*/

	return retval;
}

void filesystem_client_handler(t_Client *new_client) {
	pthread_create(&(new_client->socket_client.bool_thread.thread), NULL, (void *(*)(void *)) filesystem_thread_for_client, (void *) new_client);
	new_client->socket_client.bool_thread.running = true;
	pthread_detach(new_client->socket_client.bool_thread.thread);
}

void *filesystem_thread_for_client(t_Client *new_client) {
	log_trace(MODULE_LOGGER, "[%d] Manejador de [Cliente] %s iniciado", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);

    if(server_handshake(new_client->server->server_type, new_client->server->clients_type, new_client->socket_client.fd)) {
        if(close(new_client->socket_client.fd)) {
			report_error_close();
		}
		free(new_client);
        return NULL;
	}

	filesystem_client_handler_for_memory(new_client->socket_client.fd);

	log_trace(MODULE_LOGGER, "[%d] Manejador de [Cliente] %s finalizado", new_client->socket_client.fd, PORT_NAMES[new_client->client_type]);
    if(close(new_client->socket_client.fd)) {
		report_error_close();
	}
	free(new_client);
    return NULL;
}