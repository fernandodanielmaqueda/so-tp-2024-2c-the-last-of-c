/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef MEMORIA_SOCKET_H
#define MEMORIA_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/memory.h"
#include "commons/bitarray.h"
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "utils/module.h"
#include "utils/socket.h"
#include "memoria.h"

extern t_Connection TEMPORAL_CONNECTION_FILESYSTEM;

extern t_Server SERVER_MEMORY;

extern t_Client *CLIENT_CPU;
extern pthread_mutex_t MUTEX_CLIENT_CPU;

extern t_Shared_List SHARED_LIST_CLIENTS_KERNEL;
extern t_Shared_List SHARED_LIST_CONNECTIONS_FILESYSTEM;

void initialize_sockets(void);
void finish_sockets(void);

void memory_client_handler(t_Client *new_client);
void *memory_thread_for_client(t_Client *new_client);

#endif /* MEMORIA_SOCKET_H */