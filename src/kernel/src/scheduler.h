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

extern pthread_rwlock_t SCHEDULING_RWLOCK;

extern t_Shared_List SHARED_LIST_NEW;

extern pthread_rwlock_t ARRAY_READY_RWLOCK;

extern t_Shared_List *ARRAY_LIST_READY;
extern t_Priority PRIORITY_COUNT;

extern t_TCB *TCB_EXEC;
extern pthread_mutex_t MUTEX_EXEC;

extern t_Shared_List SHARED_LIST_BLOCKED_MEMORY_DUMP;
extern pthread_cond_t COND_BLOCKED_MEMORY_DUMP;

extern t_Shared_List SHARED_LIST_BLOCKED_IO_READY;

extern t_TCB *TCB_BLOCKED_IO_EXEC;
extern pthread_mutex_t MUTEX_BLOCKED_IO_EXEC;

extern t_Shared_List SHARED_LIST_EXIT;

extern t_Bool_Thread THREAD_LONG_TERM_SCHEDULER_NEW;
extern sem_t SEM_LONG_TERM_SCHEDULER_NEW;

extern t_Bool_Thread THREAD_LONG_TERM_SCHEDULER_EXIT;
extern sem_t SEM_LONG_TERM_SCHEDULER_EXIT;

extern bool IS_TCB_IN_CPU;
extern pthread_mutex_t MUTEX_IS_TCB_IN_CPU;
extern pthread_condattr_t CONDATTR_IS_TCB_IN_CPU;
extern pthread_cond_t COND_IS_TCB_IN_CPU;

extern t_Time QUANTUM;
extern t_Bool_Thread THREAD_QUANTUM_INTERRUPTER;
extern sem_t BINARY_QUANTUM_INTERRUPTER;

extern sem_t SEM_SHORT_TERM_SCHEDULER;
extern sem_t BINARY_SHORT_TERM_SCHEDULER;

extern bool CANCEL_IO_OPERATION;
extern pthread_mutex_t MUTEX_CANCEL_IO_OPERATION;
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
void *long_term_scheduler_exit(void);
void *quantum_interrupter(void);
void *short_term_scheduler(void);
void *io_device(void);
void *dump_memory_petitioner(void);

int remove_dump_memory_thread(pthread_t *thread);
bool dump_memory_petition_matches_pthread(t_Dump_Memory_Petition *dump_memory_petition, pthread_t *thread);
bool dump_memory_petition_matches_tcb(t_Dump_Memory_Petition *dump_memory_petition, t_TCB *tcb);

int wait_free_memory(void);
int signal_free_memory(void);

#endif // KERNEL_SCHEDULER_H