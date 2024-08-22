/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef KERNEL_INTERFACES_H
#define KERNEL_INTERFACES_H

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
#include <commons/temporal.h>
#include "utils/module.h"
#include "utils/serialize/exec_context.h"
#include "utils/socket.h"
#include "console.h"
#include "socket.h"
#include "scheduler.h"

typedef struct t_Interface {
    t_Client *client;

    char *name;
	e_IO_Type io_type;

	t_Shared_List shared_list_blocked_ready;
	t_Shared_List shared_list_blocked_exec;

    sem_t sem_scheduler;
    sem_t sem_concurrency;
    
    pthread_t thread_io_interface_dispatcher;
} t_Interface;

extern t_list *LIST_INTERFACES;
extern t_Drain_Ongoing_Resource_Sync INTERFACES_SYNC;

void *kernel_client_handler_for_io(t_Client *new_client);
void *kernel_io_interface_dispatcher(t_Interface *interface);
t_Interface *interface_create(t_Client *client);
void interface_exit(t_Interface *interface);
bool interface_name_matches(t_Interface *interface, char *name);
void interface_destroy(t_Interface *interface);

#endif // KERNEL_INTERFACES_H