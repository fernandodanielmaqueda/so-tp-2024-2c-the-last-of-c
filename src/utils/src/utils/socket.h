/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SOCKET_H
#define UTILS_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "utils/module.h"
#include "utils/send.h"

#define RETRY_CONNECTION_IN_SECONDS 10
#define MAX_CONNECTION_ATTEMPS 10

typedef struct t_Socket {
    int fd;
    t_Bool_Thread bool_thread;
} t_Socket;

typedef struct t_Connection {
    t_Socket socket_connection;
    enum e_Port_Type client_type;
    enum e_Port_Type server_type;
    char *ip;
    char *port;
} t_Connection;

typedef struct t_Server {
    t_Socket socket_listen;
    enum e_Port_Type server_type;
    enum e_Port_Type clients_type;
    char *port;
} t_Server;

typedef struct t_Client {
    t_Socket socket_client;
    enum e_Port_Type client_type;
    t_Server *server;
} t_Client;

// Server

void *server_thread_for_client(t_Client *new_client);
void *server_thread_coordinator(t_Server *server, void (*client_handler)(t_Client *));

void server_start(t_Server *server);
int server_start_try(char *port);

int server_accept(int socket_servidor);

int server_handshake(e_Port_Type server_type, e_Port_Type client_type, int fd_client);

// Client

void *client_thread_connect_to_server(t_Connection *connection);

int client_start_try(char *ip, char *port);

int wrapper_close(int *fd);

// Array de sockets

int socket_array_finish(t_Socket *sockets[]);

#endif // UTILS_SOCKET_H