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
#include "socket.h"

typedef enum e_ID_Manager_Type {
    PROCESS_ID_MANAGER_TYPE,
    THREAD_ID_MANAGER_TYPE
} e_ID_Manager_Type;

typedef struct t_ID_Manager {
    e_ID_Manager_Type id_data_type;

    void *id_counter;
    void **cb_array;
    pthread_mutex_t mutex_cb_array;
    t_list *list_released_ids; // LIFO
    pthread_mutex_t mutex_list_released_ids;
    pthread_cond_t cond_list_released_ids;
} t_ID_Manager;

typedef enum e_Process_State {
    NEW_STATE,
    READY_STATE,
    EXEC_STATE,
    BLOCKED_STATE,
	EXIT_STATE
} e_Process_State;

typedef struct t_PCB {
    t_PID PID;

    size_t size;

    t_ID_Manager thread_manager;

    t_Shared_List shared_list_mutexes;
} t_PCB;

typedef struct t_TCB {
    t_TID TID;

    t_PCB *pcb;

    char *pseudocode_filename;
    t_Priority priority;

    e_Process_State current_state;
    t_Shared_List *shared_list_state;

    t_Quantum quantum;

    t_Payload syscall_instruction;

    t_Shared_List shared_list_blocked_thread_join;
} t_TCB;

typedef enum e_Scheduling_Algorithm {
    FIFO_SCHEDULING_ALGORITHM,
    PRIORITIES_SCHEDULING_ALGORITHM,
    MLQ_SCHEDULING_ALGORITHM
} e_Scheduling_Algorithm;

#include "scheduler.h"
#include "syscalls.h"

extern char *MODULE_NAME;

extern t_log *MODULE_LOGGER;
extern char *MINIMAL_LOG_PATHNAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

extern t_ID_Manager PID_MANAGER;

extern const char *STATE_NAMES[];

extern char *SCHEDULING_ALGORITHMS[];

extern e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

int module(int, char*[]);
int initialize_global_variables(void);
int finish_global_variables(void);
int read_module_config(t_config *module_config);
int find_scheduling_algorithm(char *name, e_Scheduling_Algorithm *destination);

t_PCB *pcb_create(size_t size);
void pcb_destroy(t_PCB *pcb);

t_TCB *tcb_create(t_PCB *pcb, char *pseudocode_filename, t_Priority priority);
void tcb_destroy(t_TCB *tcb);

int id_manager_init(t_ID_Manager *id_manager, e_ID_Manager_Type id_data_type);
void id_manager_destroy(t_ID_Manager *id_manager);

int _id_assign(t_ID_Manager *id_manager, void *element, void *result);
int pid_assign(t_ID_Manager *id_manager, t_PCB *pcb, t_PID *result);
int tid_assign(t_ID_Manager *id_manager, t_TCB *tcb, t_TID *result);

int _id_release(t_ID_Manager *id_manager, size_t id, void *data);
int pid_release(t_ID_Manager *id_manager, t_PID pid);
int tid_release(t_ID_Manager *id_manager, t_TID tid);

size_t get_id_max(e_ID_Manager_Type id_manager_type);
size_t get_id_size(e_ID_Manager_Type id_manager_type);
int id_to_size(e_ID_Manager_Type id_data_type, void *source, size_t *destination);
int size_to_id(e_ID_Manager_Type id_data_type, size_t *source, void *destination);

bool pcb_matches_pid(t_PCB *pcb, t_PID *pid);
bool tcb_matches_tid(t_TCB *tcb, t_TID *tid);

void log_state_list(t_log *logger, const char *state_name, t_list *pcb_list);
void pcb_list_to_pid_string(t_list *pcb_list, char **destination);

#endif // KERNEL_H