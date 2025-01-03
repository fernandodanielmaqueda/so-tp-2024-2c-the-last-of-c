/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

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
#include "utils/socket.h"
#include "utils/timespec.h"
#include "kernel.h"
#include "transitions.h"

#define CONNECTION_MEMORY_INITIALIZER ((t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")})

extern pthread_rwlock_t RWLOCK_SCHEDULING;

extern t_Shared_List SHARED_LIST_NEW;

extern pthread_rwlock_t RWLOCK_ARRAY_READY;

extern t_Ready **ARRAY_READY;
extern t_Priority PRIORITY_COUNT;

extern t_TCB *TCB_EXEC;
extern pthread_mutex_t MUTEX_EXEC;
extern sem_t *SEM_READY;

extern t_Shared_List SHARED_LIST_BLOCKED_DUMP_MEMORY;
extern pthread_cond_t COND_BLOCKED_DUMP_MEMORY;

extern t_Shared_List SHARED_LIST_BLOCKED_IO_READY;

extern t_TCB *TCB_BLOCKED_IO_EXEC;
extern pthread_mutex_t MUTEX_BLOCKED_IO_EXEC;

extern t_Bool_Thread THREAD_LONG_TERM_SCHEDULER_NEW;
extern sem_t SEM_LONG_TERM_SCHEDULER_NEW;

extern t_Time QUANTUM;
extern t_Bool_Thread THREAD_QUANTUM_INTERRUPTER;
extern sem_t BINARY_QUANTUM_INTERRUPTER;
extern pthread_mutex_t MUTEX_QUANTUM_INTERRUPTER;
extern pthread_condattr_t CONDATTR_QUANTUM_INTERRUPTER;
extern pthread_cond_t COND_QUANTUM_INTERRUPTER;
extern bool FINISH_QUANTUM;
extern bool QUANTUM_EXPIRED;
extern bool IS_TCB_IN_CPU;

extern sem_t SEM_SHORT_TERM_SCHEDULER;
extern sem_t BINARY_SHORT_TERM_SCHEDULER;

extern bool CANCEL_IO_OPERATION;
extern pthread_mutex_t MUTEX_CANCEL_IO_OPERATION;
extern pthread_condattr_t CONDATTR_CANCEL_IO_OPERATION;
extern pthread_cond_t COND_CANCEL_IO_OPERATION;

extern t_Bool_Thread THREAD_IO_DEVICE;
extern sem_t SEM_IO_DEVICE;

extern bool FREE_MEMORY;
extern pthread_mutex_t MUTEX_FREE_MEMORY;
extern pthread_cond_t COND_FREE_MEMORY;

extern bool KILL_EXEC_TCB;
extern e_Exit_Reason KILL_EXIT_REASON;

extern bool SHOULD_REDISPATCH;

void initialize_scheduling(void);
int finish_scheduling(void);

void *long_term_scheduler_new(void);
void record_init_long_term_scheduler_new(void);
void record_finish_long_term_scheduler_new(void);

void *short_term_scheduler(void);
void record_init_short_term_scheduler(void);
void record_finish_short_term_scheduler(void);

void *quantum_interrupter(void);
void record_init_quantum_interrupter(void);
void record_finish_quantum_interrupter(void);

void *io_device(void);
void record_init_io_device(void);
void record_finish_io_device(void);

void *dump_memory_petitioner(t_Dump_Memory_Petition *dump_memory_petition);
int remove_dump_memory_thread(t_Dump_Memory_Petition *dump_memory_petition);
bool dump_memory_petition_matches_tcb(t_Dump_Memory_Petition *dump_memory_petition, t_TCB *tcb);
void record_init_dump_memory_thread(t_Dump_Memory_Petition *dump_memory_petition);
void record_finish_dump_memory_thread(t_Dump_Memory_Petition *dump_memory_petition);

int wait_free_memory(void);
int signal_free_memory(void);

#endif // KERNEL_SCHEDULER_H