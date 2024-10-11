
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
#include "kernel.h"

extern t_Shared_List SHARED_LIST_NEW;

extern t_Drain_Ongoing_Resource_Sync READY_SYNC;
extern t_Shared_List *ARRAY_LIST_READY;
extern t_Priority PRIORITY_COUNT;

extern t_Shared_List SHARED_LIST_EXEC;

extern t_Shared_List SHARED_LIST_EXIT;

extern t_PThread_Controller THREAD_LONG_TERM_SCHEDULER_NEW;
extern sem_t SEM_LONG_TERM_SCHEDULER_NEW;

extern t_PThread_Controller THREAD_LONG_TERM_SCHEDULER_EXIT;
extern sem_t SEM_LONG_TERM_SCHEDULER_EXIT;

extern t_Time QUANTUM;
extern t_PThread_Controller THREAD_QUANTUM_INTERRUPTER;
extern sem_t BINARY_QUANTUM;
extern bool QUANTUM_INTERRUPT;
extern pthread_mutex_t MUTEX_QUANTUM_INTERRUPT;

extern sem_t SEM_SHORT_TERM_SCHEDULER;
extern bool IS_TCB_IN_CPU;
extern pthread_mutex_t MUTEX_IS_TCB_IN_CPU;

extern bool FREE_MEMORY;
extern pthread_mutex_t MUTEX_FREE_MEMORY;
extern pthread_cond_t COND_FREE_MEMORY;

extern bool KILL_EXEC_PROCESS;
extern pthread_mutex_t MUTEX_KILL_EXEC_PROCESS;

extern t_TCB *EXEC_TCB;

extern bool SHOULD_REDISPATCH;

extern t_Drain_Ongoing_Resource_Sync SCHEDULING_SYNC;

void initialize_scheduling(void);
void finish_scheduling(void);

int initialize_long_term_scheduler(void);
int finish_long_term_scheduler(void);

void *long_term_scheduler_new(void *NULL_parameter);
void *long_term_scheduler_exit(void *NULL_parameter);
void *quantum_interrupter(void *NULL_parameter);
void *short_term_scheduler(void *NULL_parameter);

int wait_free_memory(void);
int signal_free_memory(void);

void* start_quantum(t_TCB *tcb);

void switch_process_state(t_TCB *tcb, e_Process_State new_state);

#endif // KERNEL_SCHEDULER_H