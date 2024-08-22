
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

typedef enum e_Scheduling_Algorithm {
    FIFO_SCHEDULING_ALGORITHM,
    RR_SCHEDULING_ALGORITHM,
    VRR_SCHEDULING_ALGORITHM
} e_Scheduling_Algorithm;

typedef struct t_Scheduling_Algorithm {
    char *name;
    t_PCB *(*function_fetcher) (void);
} t_Scheduling_Algorithm;

extern t_Shared_List SHARED_LIST_NEW;
extern t_Shared_List SHARED_LIST_READY;
extern t_Shared_List SHARED_LIST_READY_PRIORITARY;
extern t_Shared_List SHARED_LIST_EXEC;
extern t_Shared_List SHARED_LIST_EXIT;

extern pthread_t THREAD_LONG_TERM_SCHEDULER_NEW;
extern sem_t SEM_LONG_TERM_SCHEDULER_NEW;
extern pthread_t THREAD_LONG_TERM_SCHEDULER_EXIT;
extern sem_t SEM_LONG_TERM_SCHEDULER_EXIT;
extern pthread_t THREAD_SHORT_TERM_SCHEDULER;
extern sem_t SEM_SHORT_TERM_SCHEDULER;

extern int EXEC_PCB;

extern t_Scheduling_Algorithm SCHEDULING_ALGORITHMS[];

extern e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

extern t_Quantum QUANTUM;
extern pthread_t THREAD_QUANTUM_INTERRUPT;
extern pthread_mutex_t MUTEX_QUANTUM_INTERRUPT;
extern int QUANTUM_INTERRUPT;

extern t_temporal *TEMPORAL_DISPATCHED;

extern t_Drain_Ongoing_Resource_Sync SCHEDULING_SYNC;

int find_scheduling_algorithm(char *name, e_Scheduling_Algorithm *destination);
void initialize_scheduling(void);
void finish_scheduling(void);
void initialize_long_term_scheduler(void);
void initialize_short_term_scheduler(void);
void *long_term_scheduler_new(void *NULL_parameter);
void *long_term_scheduler_exit(void *NULL_parameter);
void *short_term_scheduler(void *NULL_parameter);
void* start_quantum(t_PCB *pcb);
t_PCB *FIFO_scheduling_algorithm(void);
t_PCB *RR_scheduling_algorithm(void);
t_PCB *VRR_scheduling_algorithm(void);
void switch_process_state(t_PCB* pcb, e_Process_State NEW_STATE);

#endif // KERNEL_SCHEDULER_H