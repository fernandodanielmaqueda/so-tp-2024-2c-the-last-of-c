/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef KERNEL_SOCKET_H
#define KERNEL_SOCKET_H

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
#include "kernel.h"

//extern t_Connection TEMPORAL_CONNECTION_MEMORY;

extern t_log *SOCKET_LOGGER;

extern t_Server COORDINATOR_IO;
extern t_Connection CONNECTION_MEMORY;
extern t_Connection CONNECTION_CPU_DISPATCH;
extern t_Connection CONNECTION_CPU_INTERRUPT;

int initialize_sockets(void);
int finish_sockets(void);
void *kernel_start_server_for_io(t_Server *server);

#endif /* KERNEL_SOCKET_H */