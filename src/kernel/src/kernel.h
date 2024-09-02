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

typedef unsigned int t_Priority;

typedef struct t_PCB {
    t_PID PID;
    t_ID_Manager thread_manager;
    t_list *list_mutexes;
} t_PCB;

typedef struct t_TCB {
    t_TID TID;

    t_PCB *pcb;

    e_Process_State current_state;
    t_Shared_List *shared_list_state;

    t_Priority priority;

    t_Quantum quantum;

    t_Payload syscall_instruction;
} t_TCB;


#include "scheduler.h"
#include "syscalls.h"

extern char *MODULE_NAME;

extern t_log *MODULE_LOGGER;
extern char *MINIMAL_LOG_PATHNAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

extern t_ID_Manager PID_MANAGER;

extern const char *STATE_NAMES[];

extern const char *EXIT_REASONS[];

int module(int, char*[]);
void initialize_global_variables(void);
void finish_global_variables(void);
void read_module_config(t_config *module_config);

t_PCB *pcb_create(void);
void pcb_destroy(t_PCB *pcb);

t_TCB *tcb_create(t_PCB *pcb);
void tcb_destroy(t_TCB *tcb);

void id_manager_init(t_ID_Manager *id_manager, e_ID_Manager_Type id_data_type);
void id_manager_destroy(t_ID_Manager *id_manager);

size_t _id_assign(t_ID_Manager *id_manager, void *data);
t_PID pid_assign(t_ID_Manager *id_manager, t_PCB *pcb);
t_TID tid_assign(t_ID_Manager *id_manager, t_TCB *tcb);

void _id_release(t_ID_Manager *id_manager, size_t id, void *data);
void pid_release(t_ID_Manager *id_manager, t_PID pid);
void tid_release(t_ID_Manager *id_manager, t_TID tid);

size_t get_id_max(e_ID_Manager_Type id_manager_type);
size_t get_id_size(e_ID_Manager_Type id_manager_type);
void id_to_size(e_ID_Manager_Type id_data_type, size_t *destination, void *source);
void size_to_id(e_ID_Manager_Type id_data_type, void *destination, size_t *source);

bool pcb_matches_pid(t_PCB *pcb, t_PID *pid);
bool tcb_matches_tid(t_TCB *tcb, t_TID *tid);

void log_state_list(t_log *logger, const char *state_name, t_list *pcb_list);
void pcb_list_to_pid_string(t_list *pcb_list, char **destination);

#endif /* KERNEL_H */