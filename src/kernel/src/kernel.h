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
#include "console.h"
#include "socket.h"

typedef enum e_Process_State {
    NEW_STATE,
    READY_STATE,
    EXEC_STATE,
    BLOCKED_STATE,
	EXIT_STATE
} e_Process_State;

typedef struct t_PCB {
    t_Exec_Context exec_context;

    e_Process_State current_state;
    t_Shared_List *shared_list_state;

    t_list *assigned_resources;

    t_Payload io_operation;

    e_Exit_Reason exit_reason;
} t_PCB;

#include "scheduler.h"
#include "resources.h"
#include "interfaces.h"
#include "syscalls.h"

extern char *MODULE_NAME;

extern t_log *MODULE_LOGGER;
extern char *MINIMAL_LOG_PATHNAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

extern t_PID PID_COUNTER;
extern pthread_mutex_t MUTEX_PID_COUNTER;
extern t_PCB **PCB_ARRAY;
extern pthread_mutex_t MUTEX_PCB_ARRAY;
extern t_list *LIST_RELEASED_PIDS; // LIFO
extern pthread_mutex_t MUTEX_LIST_RELEASED_PIDS;
extern pthread_cond_t COND_LIST_RELEASED_PIDS;

extern const char *STATE_NAMES[];

extern const char *EXIT_REASONS[];

int module(int, char*[]);
void initialize_mutexes(void);
void finish_mutexes(void);
void initialize_semaphores(void);
void finish_semaphores(void);
void read_module_config(t_config *module_config);

t_PCB *pcb_create(void);
void pcb_destroy(t_PCB *pcb);

t_PID pid_assign(t_PCB *pcb);
void pid_release(t_PID pid);

bool pcb_matches_pid(t_PCB *pcb, t_PID *pid);

void log_state_list(t_log *logger, const char *state_name, t_list *pcb_list);
void pcb_list_to_pid_string(t_list *pcb_list, char **destination);

#endif /* KERNEL_H */