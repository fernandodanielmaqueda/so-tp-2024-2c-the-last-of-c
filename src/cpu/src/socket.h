/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef CPU_SOCKET_H
#define CPU_SOCKET_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <semaphore.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/memory.h"
#include "commons/bitarray.h"
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "utils/module.h"
#include "utils/socket.h"
#include "cpu.h"

extern t_Server SERVER_CPU_DISPATCH;
extern t_Server SERVER_CPU_INTERRUPT;

extern t_Client CLIENT_KERNEL_CPU_DISPATCH;
extern t_Client CLIENT_KERNEL_CPU_INTERRUPT;

extern t_Connection CONNECTION_MEMORY;

int initialize_sockets(void);
int finish_sockets(void);
void *cpu_start_server_for_kernel(t_Client *new_client);

#endif // CPU_SOCKET_H