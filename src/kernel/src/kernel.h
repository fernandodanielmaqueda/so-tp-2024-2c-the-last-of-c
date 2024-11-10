/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef KERNEL_H
#define KERNEL_H

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
#include "utils/send.h"
#include "utils/socket.h"

typedef struct t_PID_Manager {
    t_PID size;
    void **array;
    t_PID counter;
    pthread_mutex_t mutex;
} t_PID_Manager;

typedef struct t_TID_Manager {
    t_TID size;
    void **array;
    t_TID counter;
    pthread_mutex_t mutex;
} t_TID_Manager;

typedef enum e_Process_State {
    NEW_STATE,
    READY_STATE,
    EXEC_STATE,

    BLOCKED_JOIN_STATE,
    BLOCKED_MUTEX_STATE,
    BLOCKED_DUMP_STATE,
    BLOCKED_IO_READY_STATE,
    BLOCKED_IO_EXEC_STATE,

	EXIT_STATE
} e_Process_State;

typedef enum e_Exit_Reason {
    UNEXPECTED_ERROR_EXIT_REASON,

    INVALID_RESOURCE_EXIT_REASON,
    SEGMENTATION_FAULT_EXIT_REASON,
    PROCESS_EXIT_EXIT_REASON,
    THREAD_EXIT_EXIT_REASON,
    THREAD_CANCEL_EXIT_REASON
} e_Exit_Reason;

typedef struct t_PCB {
    t_PID PID;

    size_t size;

    t_TID_Manager thread_manager;

    pthread_rwlock_t rwlock_resources;
    t_dictionary *dictionary_resources;
} t_PCB;

typedef struct t_TCB {
    t_TID TID;

    t_PCB *pcb;

    char *pseudocode_filename;
    t_Priority priority;

    e_Process_State current_state;
    void *location;

    t_Time quantum;

    t_Payload syscall_instruction;

    t_Shared_List shared_list_blocked_thread_join;

    t_dictionary *dictionary_assigned_resources;

    e_Exit_Reason exit_reason;
} t_TCB;

typedef enum e_Scheduling_Algorithm {
    FIFO_SCHEDULING_ALGORITHM,
    PRIORITIES_SCHEDULING_ALGORITHM,
    MLQ_SCHEDULING_ALGORITHM
} e_Scheduling_Algorithm;

typedef struct t_Dump_Memory_Petition {
    t_Bool_Thread bool_thread;
    t_TCB *tcb;
} t_Dump_Memory_Petition;

#include "resources.h"
#include "socket.h"
#include "scheduler.h"
#include "syscalls.h"

extern char *MODULE_NAME;

extern t_log *MODULE_LOGGER;
extern char *MINIMAL_LOG_PATHNAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

extern t_PID_Manager PID_MANAGER;

extern const char *STATE_NAMES[];

extern const char *EXIT_REASONS[];

extern char *SCHEDULING_ALGORITHMS[];

extern e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

int module(int, char*[]);
int read_module_config(t_config *module_config);
int find_scheduling_algorithm(char *name, e_Scheduling_Algorithm *destination);

t_PCB *pcb_create(size_t size);
int pcb_destroy(t_PCB *pcb);

t_TCB *tcb_create(t_PCB *pcb, char *pseudocode_filename, t_Priority priority);
int tcb_destroy(t_TCB *tcb);

int pid_manager_init(t_PID_Manager *id_manager);
int tid_manager_init(t_TID_Manager *id_manager);

int pid_manager_destroy(t_PID_Manager *id_manager);
int tid_manager_destroy(t_TID_Manager *id_manager);

int pid_assign(t_PID_Manager *id_manager, t_PCB *pcb, t_PID *result);
int tid_assign(t_TID_Manager *id_manager, t_TCB *tcb, t_TID *result);

int pid_release(t_PID_Manager *id_manager, t_PID pid);
int tid_release(t_TID_Manager *id_manager, t_TID tid);


bool pcb_matches_pid(t_PCB *pcb, t_PID *pid);
bool tcb_matches_tid(t_TCB *tcb, t_TID *tid);

int new_process(size_t size, char *pseudocode_filename, t_Priority priority);
int thread_create(t_PCB *pcb, t_TID tid);

int array_list_ready_init(void);
int array_list_ready_update(t_Priority priority);
int array_list_ready_resize(t_Priority priority);
int array_list_ready_destroy(void);

void log_state_list(t_log *logger, const char *state_name, t_list *pcb_list);
void pcb_list_to_pid_string(t_list *pcb_list, char **destination);
void tcb_list_to_pid_tid_string(t_list *tcb_list, char **destination);
void dump_memmory_list_to_pid_tid_string(t_list *dump_memory_list, char **destination);

#endif // KERNEL_H