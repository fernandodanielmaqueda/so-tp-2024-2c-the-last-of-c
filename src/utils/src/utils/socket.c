/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

// Server

void *server_thread_coordinator(t_Server *server, void (*client_handler)(t_Client *)) {

	int fd_new_client;
	t_Client *new_client;

	server_start(server);

	while(1) {
		log_trace(SOCKET_LOGGER, "[%d] Esperando [Cliente] %s en Puerto: %s", server->fd_listen, PORT_NAMES[server->clients_type], server->port);
		fd_new_client = server_accept(server->fd_listen);

		if(fd_new_client == -1) {
			log_warning(SOCKET_LOGGER, "[%d] Fallo al aceptar [Cliente] %s", server->fd_listen, PORT_NAMES[server->clients_type]);
			continue;
		}

		new_client = malloc(sizeof(t_Client));
		if(new_client == NULL) {
			log_warning(SOCKET_LOGGER, "malloc: No se pudieron reservar %zu bytes para un nuevo cliente", sizeof(t_Client));
			close(fd_new_client);
			continue;
		}

		log_trace(SOCKET_LOGGER, "[%d] Aceptado [Cliente] %s [%d]", server->fd_listen, PORT_NAMES[server->clients_type], fd_new_client);
		new_client->fd_client = fd_new_client;
		new_client->client_type = server->clients_type;
		new_client->server = server;
    client_handler(new_client);
	}

	return NULL;
}

void server_start(t_Server *server) {

  while(1) {
    log_info(SOCKET_LOGGER, "Intentando iniciar [Servidor] %s en Puerto: %s...", PORT_NAMES[server->server_type], server->port);
    server->fd_listen = server_start_try(server->port);

    if(server->fd_listen != -1)
      break;

    log_warning(SOCKET_LOGGER, "No se pudo iniciar [Servidor] %s en Puerto: %s. Reintentando en %d segundos...", PORT_NAMES[server->server_type], server->port, RETRY_CONNECTION_IN_SECONDS);
    sleep(RETRY_CONNECTION_IN_SECONDS);

  }

  log_trace(SOCKET_LOGGER, "[%d] Escuchando [Servidor] %s en Puerto: %s", server->fd_listen, PORT_NAMES[server->server_type], server->port);
}

int server_start_try(char *port) {

	struct addrinfo hints;
  struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET para IPv4 unicamente
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(NULL, port, &hints, &result);
  if (status) {
    log_warning(SOCKET_LOGGER, "Funcion getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }

  int fd_server;

  for(rp = result ; rp != NULL ; rp = rp->ai_next) {
    fd_server = socket(
      rp->ai_family,
      rp->ai_socktype,
      rp->ai_protocol
    );

    if(fd_server == -1) {
      log_warning(SOCKET_LOGGER, "Funcion socket: %s\n", strerror(errno));
      continue; // This one failed
    }

    // Permite reutilizar el puerto inmediatamente después de cerrar el socket
    if(setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, (socklen_t) sizeof(int)) == -1) {
        log_warning(SOCKET_LOGGER, "Function setsockopt: %s\n", strerror(errno));
        close(fd_server);
        continue; // This one failed
    }

    if(bind(fd_server, rp->ai_addr, rp->ai_addrlen) == -1) {
      log_warning(SOCKET_LOGGER, "Funcion bind: %s\n", strerror(errno));
      close(fd_server);
      continue; // This one failed
    }
    
      break; // Until one succeeds    
  }
	
  freeaddrinfo(result); // No longer needed

  if (rp == NULL) { // No address succeeded
    return -1;
  }

	// Escuchamos las conexiones entrantes
	if (listen(fd_server, SOMAXCONN) == -1) {
		log_warning(SOCKET_LOGGER, "Funcion listen: %s\n", strerror(errno));
		return -1;
	}

	return fd_server;
}

int server_accept(int fd_server) {
  // Syscall bloqueante que se queda esperando hasta que llegue un nuevo cliente
	int fd_client = accept(fd_server, NULL, NULL);

	if(fd_client == -1) {
      log_warning(SOCKET_LOGGER, "Funcion accept: %s\n", strerror(errno));
  }

	return fd_client;
}

int server_handshake(e_Port_Type server_type, e_Port_Type client_type, int fd_client) {

  e_Port_Type port_type;

  if(receive_port_type(&port_type, false)) {
    log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Cliente] %s", fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
    return -1;
  }

  if(port_type != client_type) {
    log_warning(SOCKET_LOGGER, "[%d] No reconocido Handshake de [Cliente] %s", fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
    send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, fd_client);
    return -1;
  }

  if(send_port_type(server_type, fd_client)) {
    log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Cliente] %s", fd_client, PORT_NAMES[client_type]);
    return -1;
  }

  log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", fd_client, PORT_NAMES[client_type]);
  return 0;
}

// Client

void *client_thread_connect_to_server(t_Connection *connection) {

  log_trace(MODULE_LOGGER, "Conexión con [Servidor] %s iniciada", PORT_NAMES[connection->server_type]);

  e_Port_Type port_type;

  while(1) {
    while(1) {
      log_info(SOCKET_LOGGER, "Intentando conectar con [Servidor] %s en IP: %s - Puerto: %s...", PORT_NAMES[connection->server_type], connection->ip, connection->port);
      connection->fd_connection = client_start_try(connection->ip, connection->port);

      if(connection->fd_connection != -1)
        break;

      log_warning(SOCKET_LOGGER, "No se pudo conectar con [Servidor] %s en IP: %s - Puerto: %s. Reintentando en %d segundos...", PORT_NAMES[connection->server_type], connection->ip, connection->port, RETRY_CONNECTION_IN_SECONDS);
      sleep(RETRY_CONNECTION_IN_SECONDS);
    }

    log_trace(SOCKET_LOGGER, "[%d] Conectado con [Servidor] %s", connection->fd_connection, PORT_NAMES[connection->server_type]);

    // Handshake

    if(send_port_type(connection->client_type, connection->fd_connection)) {
      log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Servidor] %s. Reintentando en %d segundos...", connection->fd_connection, PORT_NAMES[connection->server_type], RETRY_CONNECTION_IN_SECONDS);
      close(connection->fd_connection);
      sleep(RETRY_CONNECTION_IN_SECONDS);
      continue;
    }
    if(receive_port_type(&port_type, connection->fd_connection)) {
      log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Servidor] %s. Reintentando en %d segundos...", connection->fd_connection, PORT_NAMES[connection->server_type], RETRY_CONNECTION_IN_SECONDS);
      close(connection->fd_connection);
      sleep(RETRY_CONNECTION_IN_SECONDS);
      continue;
    }

    if(port_type != connection->server_type) {
      log_warning(SOCKET_LOGGER, "[%d] No reconocido Handshake de [Servidor] %s. Reintentando en %d segundos...", connection->fd_connection, PORT_NAMES[connection->server_type], RETRY_CONNECTION_IN_SECONDS);
      close(connection->fd_connection);
      sleep(RETRY_CONNECTION_IN_SECONDS);
      continue;
    }

    log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Servidor] %s", connection->fd_connection, PORT_NAMES[connection->server_type]);
    break;
  }

  log_trace(MODULE_LOGGER, "Conexión con [Servidor] %s finalizada", PORT_NAMES[connection->server_type]);

  return NULL;
}

int client_start_try(char *ip, char *port) {

	struct addrinfo hints;
	struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET para IPv4 unicamente
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(ip, port, &hints, &result);
  if (status) {
    log_warning(SOCKET_LOGGER, "Funcion getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }

	// Ahora vamos a crear el socket.
	int fd_client;

  for(rp = result ; rp != NULL ; rp = rp->ai_next) {
    fd_client = socket(
      rp->ai_family,
      rp->ai_socktype,
      rp->ai_protocol
    );

    if(fd_client == -1) {
      log_warning(SOCKET_LOGGER, "Funcion socket: %s\n", strerror(errno));
      continue; /* This one failed */
    }

    if(connect(fd_client, rp->ai_addr, rp->ai_addrlen) == 0)
      break; /* Until one succeeds */

    log_warning(SOCKET_LOGGER, "Funcion connect: %s\n", strerror(errno));
    close(fd_client);
  }
	
  freeaddrinfo(result); /* No longer needed */

  if (rp == NULL) { /* No address succeeded */
    return -1;
  }
	
  return fd_client;
}

bool client_matches_pthread(t_Client *client, pthread_t *thread) {
    return pthread_equal(client->thread_client_handler, *thread);
}