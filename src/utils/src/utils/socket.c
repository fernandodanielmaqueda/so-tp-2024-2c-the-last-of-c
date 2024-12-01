/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

// Server

void *server_thread_for_client(t_Client *new_client) {

  log_trace(MODULE_LOGGER, "Inicio de [Servidor] %s para [Cliente] %s en Puerto: %s", PORT_NAMES[new_client->server->server_type], PORT_NAMES[new_client->server->clients_type], new_client->server->port);

  server_start(new_client->server);

  while(1) {
    while(1) {
      log_trace(SOCKET_LOGGER, "[%d] Esperando [Cliente] %s en Puerto: %s", new_client->server->socket_listen.fd, PORT_NAMES[new_client->server->clients_type], new_client->server->port);
      new_client->socket_client.fd = server_accept(new_client->server->socket_listen.fd);

      if((new_client->socket_client.fd) != -1)
          break;

      log_warning(SOCKET_LOGGER, "[%d] Fallo al aceptar [Cliente] %s", new_client->server->socket_listen.fd, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
    }

    log_trace(SOCKET_LOGGER, "[%d] Aceptado [Cliente] %s [%d]", new_client->server->socket_listen.fd, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE], new_client->socket_client.fd);

    if(server_handshake(new_client->server->server_type, new_client->server->clients_type, new_client->socket_client.fd)) {
      if(close(new_client->socket_client.fd)) {
        report_error_close();
      }
      continue;
    }
    
    break;
  }

  log_debug(SOCKET_LOGGER, "[%d] Cierre de [Servidor] %s para Cliente [%s] en Puerto: %s", new_client->server->socket_listen.fd, PORT_NAMES[new_client->server->server_type], PORT_NAMES[new_client->server->clients_type], new_client->server->port);
  if(close(new_client->server->socket_listen.fd)) {
    report_error_close();
  }

  return NULL;
}

void *server_thread_coordinator(t_Server *server, void (*client_handler)(t_Client *)) {

  int retval = 0;
	int fd_new_client;
	t_Client *new_client;

  pthread_cleanup_push((void (*)(void *)) wrapper_close, (void *) &(server->socket_listen.fd));
	server_start(server);

	while(1) {
		log_trace(SOCKET_LOGGER, "[%d] Esperando [Cliente] %s en Puerto: %s", server->socket_listen.fd, PORT_NAMES[server->clients_type], server->port);
		fd_new_client = server_accept(server->socket_listen.fd);

		if(fd_new_client == -1) {
			log_warning(SOCKET_LOGGER, "[%d] Fallo al aceptar [Cliente] %s", server->socket_listen.fd, PORT_NAMES[server->clients_type]);
			continue;
		}

    pthread_cleanup_push((void (*)(void *)) wrapper_close, (void *) &fd_new_client);

		new_client = malloc(sizeof(t_Client));
		if(new_client == NULL) {
			log_warning(SOCKET_LOGGER, "malloc: No se pudieron reservar %zu bytes para un nuevo cliente", sizeof(t_Client));
			retval = -1;
			goto cleanup_fd_new_client;
		}
    pthread_cleanup_push((void (*)(void *)) free, new_client);

    log_trace(SOCKET_LOGGER, "[%d] Aceptado [Cliente] %s [%d]", server->socket_listen.fd, PORT_NAMES[server->clients_type], fd_new_client);

    new_client->socket_client.fd = fd_new_client;
    new_client->socket_client.bool_thread.running = false;
    new_client->client_type = server->clients_type;
    new_client->server = server;

    pthread_cleanup_pop(retval); // new_client

    cleanup_fd_new_client:
    pthread_cleanup_pop(retval); // fd_new_client

    if(retval) {
      continue;
    }

    client_handler(new_client);
	}

  pthread_cleanup_pop(1); // server->socket_listen.fd

	return NULL;
}

void server_start(t_Server *server) {

  while(1) {
    log_info(SOCKET_LOGGER, "Intentando iniciar [Servidor] %s en Puerto: %s...", PORT_NAMES[server->server_type], server->port);
    server->socket_listen.fd = server_start_try(server->port);

    if((server->socket_listen.fd) != -1)
      break;

    log_warning(SOCKET_LOGGER, "No se pudo iniciar [Servidor] %s en Puerto: %s. Reintentando en %d segundos...", PORT_NAMES[server->server_type], server->port, RETRY_CONNECTION_IN_SECONDS);
    sleep(RETRY_CONNECTION_IN_SECONDS);

  }

  log_debug(SOCKET_LOGGER, "[%d] Escuchando [Servidor] %s en Puerto: %s", server->socket_listen.fd, PORT_NAMES[server->server_type], server->port);
}

int server_start_try(char *port) {

	struct addrinfo hints;
  struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET para IPv4 unicamente
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(NULL, port, &hints, &result);
  if(status) {
    log_warning(SOCKET_LOGGER, "getaddrinfo: %s", gai_strerror(status));
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
      log_warning(SOCKET_LOGGER, "socket: %s", strerror(errno));
      continue; // This one failed
    }

    // Permite reutilizar el puerto inmediatamente después de cerrar el socket
    if(setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, (socklen_t) sizeof(int)) == -1) {
        log_warning(SOCKET_LOGGER, "setsockopt: %s", strerror(errno));
        if(close(fd_server)) {
          report_error_close();
        }
        continue; // This one failed
    }

    if(bind(fd_server, rp->ai_addr, rp->ai_addrlen) == -1) {
      log_warning(SOCKET_LOGGER, "bind: %s", strerror(errno));
      if(close(fd_server)) {
        report_error_close();
      }
      continue; // This one failed
    }
    
      break; // Until one succeeds    
  }
	
  freeaddrinfo(result); // No longer needed

  if(rp == NULL) { // No address succeeded
    return -1;
  }

	// Escuchamos las conexiones entrantes
	if(listen(fd_server, SOMAXCONN) == -1) {
		log_warning(SOCKET_LOGGER, "listen: %s", strerror(errno));
		return -1;
	}

	return fd_server;
}

int server_accept(int fd_server) {
  // Syscall bloqueante que se queda esperando hasta que llegue un nuevo cliente
	int fd_client = accept(fd_server, NULL, NULL);

	if(fd_client == -1) {
      log_warning(SOCKET_LOGGER, "accept: %s", strerror(errno));
  }

	return fd_client;
}

int server_handshake(e_Port_Type server_type, e_Port_Type client_type, int fd_client) {

  e_Port_Type port_type;

  if(receive_port_type(&port_type, fd_client)) {
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
      connection->socket_connection.fd = client_start_try(connection->ip, connection->port);

      if((connection->socket_connection.fd) != -1)
        break;

      log_warning(SOCKET_LOGGER, "No se pudo conectar con [Servidor] %s en IP: %s - Puerto: %s. Reintentando en %d segundos...", PORT_NAMES[connection->server_type], connection->ip, connection->port, RETRY_CONNECTION_IN_SECONDS);
      sleep(RETRY_CONNECTION_IN_SECONDS);
    }

    log_trace(SOCKET_LOGGER, "[%d] Conectado con [Servidor] %s", connection->socket_connection.fd, PORT_NAMES[connection->server_type]);

    // Handshake

    if(send_port_type(connection->client_type, connection->socket_connection.fd)) {
      log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Servidor] %s. Reintentando en %d segundos...", connection->socket_connection.fd, PORT_NAMES[connection->server_type], RETRY_CONNECTION_IN_SECONDS);
      if(close(connection->socket_connection.fd)) {
        report_error_close();
      }
      sleep(RETRY_CONNECTION_IN_SECONDS);
      continue;
    }
    if(receive_port_type(&port_type, connection->socket_connection.fd)) {
      log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Servidor] %s. Reintentando en %d segundos...", connection->socket_connection.fd, PORT_NAMES[connection->server_type], RETRY_CONNECTION_IN_SECONDS);
      if(close(connection->socket_connection.fd)) {
        report_error_close();
      }
      sleep(RETRY_CONNECTION_IN_SECONDS);
      continue;
    }

    if(port_type != connection->server_type) {
      log_warning(SOCKET_LOGGER, "[%d] No reconocido Handshake de [Servidor] %s. Reintentando en %d segundos...", connection->socket_connection.fd, PORT_NAMES[connection->server_type], RETRY_CONNECTION_IN_SECONDS);
      if(close(connection->socket_connection.fd)) {
        report_error_close();
      }
      sleep(RETRY_CONNECTION_IN_SECONDS);
      continue;
    }

    log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Servidor] %s", connection->socket_connection.fd, PORT_NAMES[connection->server_type]);
    break;
  }

  log_trace(MODULE_LOGGER, "[%d] Conexión con [Servidor] %s exitosa", connection->socket_connection.fd, PORT_NAMES[connection->server_type]);

  return NULL;
}

int client_start_try(char *ip, char *port) {

	struct addrinfo hints;
	struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET para IPv4 unicamente
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(ip, port, &hints, &result);
  if(status) {
    log_warning(SOCKET_LOGGER, "getaddrinfo: %s", gai_strerror(status));
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
      log_warning(SOCKET_LOGGER, "socket: %s", strerror(errno));
      continue; /* This one failed */
    }

    if(connect(fd_client, rp->ai_addr, rp->ai_addrlen) == 0)
      break; /* Until one succeeds */

    log_warning(SOCKET_LOGGER, "connect: %s", strerror(errno));
    if(close(fd_client)) {
      report_error_close();
    }
  }

  freeaddrinfo(result); /* No longer needed */

  if(rp == NULL) { /* No address succeeded */
    return -1;
  }
	
  return fd_client;
}

int wrapper_close(int *fd) {
	if(*fd < 0) {
		return 0;
	}

	if(close(*fd)) {
		report_error_close();
		return -1;
	}

	return 0;
}

int socket_array_finish(t_Socket *sockets[]) {
  int retval = 0, status;
  register unsigned int i;

  for(i = 0; sockets[i] != NULL; i++) {
    if(sockets[i]->bool_thread.running) {
      if((status = pthread_cancel(sockets[i]->bool_thread.thread))) {
        report_error_pthread_cancel(status);
        retval = -1;
      }
    }
  }

  for(i = 0; sockets[i] != NULL; i++) {
    if(sockets[i]->bool_thread.running) {
      if((status = pthread_join(sockets[i]->bool_thread.thread, NULL))) {
        report_error_pthread_join(status);
        retval = -1;
      }
    }
  }

  for(i = 0; sockets[i] != NULL; i++) {
    if(sockets[i]->fd >= 0) {
      if(close(sockets[i]->fd)) {
        report_error_close();
        retval = -1;
      }
      sockets[i]->fd = -1;
    }
  }

  return retval;
}