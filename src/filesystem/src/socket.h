/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef FILESYSTEM_SOCKET_H
#define FILESYSTEM_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include "commons/collections/dictionary.h"
#include "utils/module.h"
#include "utils/serialize/exec_context.h"
#include "utils/socket.h"
#include "filesystem.h"

extern t_Logger SOCKET_LOGGER;

extern t_Server SERVER_FILESYSTEM;

extern t_Shared_List SHARED_LIST_CLIENTS;
extern pthread_cond_t COND_CLIENTS;

void filesystem_client_handler(t_Client *new_client);
void *filesystem_thread_for_client(t_Client *new_client);
int remove_client_thread(t_Client *client);

int wait_client_threads(void);
int signal_client_threads(void);

#endif // FILESYSTEM_SOCKET_H