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

extern t_log *MODULE_LOGGER;
extern t_log *SOCKET_LOGGER;
extern t_config *MODULE_CONFIG;

extern t_PID PID;
extern t_TID TID;
extern t_Exec_Context EXEC_CONTEXT;
extern size_t BASE;
extern size_t LIMIT;
extern pthread_mutex_t MUTEX_EXEC_CONTEXT;

extern int EXECUTING;
extern pthread_mutex_t MUTEX_EXECUTING;

extern e_Eviction_Reason EVICTION_REASON;

extern e_Kernel_Interrupt KERNEL_INTERRUPT;
extern pthread_mutex_t MUTEX_KERNEL_INTERRUPT;

extern int SYSCALL_CALLED;
extern t_Payload SYSCALL_INSTRUCTION;

#define MAX_CPU_INSTRUCTION_ARGUMENTS 1 + 5

int module(int, char*[]);
void initialize_mutexes(void);
void finish_mutexes(void);
void initialize_semaphores(void);
void finish_semaphores(void);
void read_module_config(t_config *module_config);
void initialize_sockets(void);
void finish_sockets(void);
void *cpu_dispatch_start_server_for_kernel(void *server_parameter);
void *cpu_interrupt_start_server_for_kernel(void *server_parameter);
void instruction_cycle(void);
void *kernel_cpu_interrupt_handler(void *NULL_parameter);
t_list* mmu(t_PID pid, size_t logical_address, size_t bytes_contenido);
void cpu_fetch_next_instruction(char **line);
/*
void request_frame_memory(t_PID pid, size_t page_number);
void attend_write(t_PID pid, t_list *list_physical_addresses, char *source, size_t bytes);
void attend_read(t_PID pid, t_list *list_physical_addresses, void *destination, size_t bytes);
void attend_copy(t_PID pid, t_list *list_physical_addresses_origin, t_list *list_physical_addresses_destination, size_t bytes);
*/

#endif // CPU_H