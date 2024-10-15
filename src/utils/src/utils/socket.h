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

typedef struct t_Connection {
    int fd_connection;
    enum e_Port_Type client_type;
    enum e_Port_Type server_type;
    char *ip;
    char *port;
    pthread_t thread_connection;
} t_Connection;

typedef struct t_Server {
    int fd_listen;
    enum e_Port_Type server_type;
    enum e_Port_Type clients_type;
    char *port;
    pthread_t thread_server;
} t_Server;

typedef struct t_Client {
    int fd_client;
    enum e_Port_Type client_type;
    pthread_t thread_client_handler;
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

bool client_matches_pthread(t_Client *client, pthread_t *thread);

#endif // UTILS_SOCKET_H