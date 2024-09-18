/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/memory.h"
#include "commons/bitarray.h"
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "utils/module.h"
#include "utils/arguments.h"
#include "utils/send.h"
#include "utils/socket.h"
#include "socket.h"
#include "opcodes.h"
#include "registers.h"

extern char *MODULE_NAME;

extern t_log *MODULE_LOGGER;
extern char *MINIMAL_LOG_PATHNAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

extern t_PID PID;
extern t_TID TID;
extern t_Exec_Context EXEC_CONTEXT;
extern size_t BASE;
extern size_t LIMIT;
extern pthread_mutex_t MUTEX_EXEC_CONTEXT;

extern bool EXECUTING;
extern pthread_mutex_t MUTEX_EXECUTING;

extern e_Eviction_Reason EVICTION_REASON;

extern e_Kernel_Interrupt KERNEL_INTERRUPT;
extern pthread_mutex_t MUTEX_KERNEL_INTERRUPT;

extern bool SYSCALL_CALLED;
extern t_Payload SYSCALL_INSTRUCTION;

#define MAX_CPU_INSTRUCTION_ARGUMENTS 1 + 5

int module(int, char*[]);

int initialize_global_variables(void);
int finish_global_variables(void);

int read_module_config(t_config *module_config);

void instruction_cycle(void);

void *kernel_cpu_interrupt_handler(void *NULL_parameter);

int cpu_fetch_next_instruction(char **line);

int mmu(size_t logical_address, size_t bytes, size_t *destination);

int write_memory(size_t physical_address, void *source, size_t bytes);
int read_memory(size_t physical_address, void *destination, size_t bytes);

#endif // CPU_H