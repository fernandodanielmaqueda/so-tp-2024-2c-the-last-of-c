/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_MODULE_H
#define UTILS_MODULE_H

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include "commons/log.h"
#include "commons/config.h"

typedef struct t_Bool_Thread {
    pthread_t thread;
    bool running;
} t_Bool_Thread;

typedef struct t_Shared_List {
    t_list *list;
    pthread_mutex_t mutex;
} t_Shared_List;

typedef struct t_Bool_Join_Thread {
    bool *join;
    pthread_t *thread;
} t_Bool_Join_Thread;

extern pthread_t THREAD_SIGNAL_MANAGER;

extern char *MODULE_NAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

extern t_log_level LOG_LEVEL;

extern t_log *MINIMAL_LOGGER;
extern char *MINIMAL_LOG_PATHNAME;
extern pthread_mutex_t MUTEX_MINIMAL_LOGGER;

extern t_log *MODULE_LOGGER;
extern char *MODULE_LOG_PATHNAME;
extern pthread_mutex_t MUTEX_MODULE_LOGGER;

extern t_log *SOCKET_LOGGER;
extern char *SOCKET_LOG_PATHNAME;
extern pthread_mutex_t MUTEX_SOCKET_LOGGER;

extern t_log *SERIALIZE_LOGGER;
extern char *SERIALIZE_LOG_PATHNAME;
extern pthread_mutex_t MUTEX_SERIALIZE_LOGGER;

#define PTHREAD_SETCANCELSTATE_DISABLE() \
    do {                         \
        int __old_cancel_state;  \
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &__old_cancel_state)

#define PTHREAD_SETCANCELSTATE_OLDSTATE()                          \
        pthread_setcancelstate(__old_cancel_state, NULL); \
    } while(0)

void *signal_manager(pthread_t *thread_to_cancel);

int initialize_configs(char *pathname);
void finish_configs(void);
bool config_has_properties(t_config *config, ...);
extern int read_module_config(t_config *); // Se debe implementar en cada módulo

int initialize_loggers(void);
int finish_loggers(void);

int initialize_logger(t_log **logger, char *pathname, char *module_name);
int finish_logger(t_log **logger);

void log_error_close(void);

void log_error_fclose(void);

void log_error_sem_init(void);
void log_error_sem_destroy(void);
void log_error_sem_wait(void);
void log_error_sem_post(void);

void log_error_pthread_mutex_init(int status);
void log_error_pthread_mutex_destroy(int status);
void log_error_pthread_mutex_lock(int statusoid);
void log_error_pthread_mutex_unlock(int status);

void log_error_pthread_rwlock_init(int status);
void log_error_pthread_rwlock_destroy(int status);
void log_error_pthread_rwlock_wrlock(int status);
void log_error_pthread_rwlock_rdlock(int status);
void log_error_pthread_rwlock_unlock(int status);

void log_error_pthread_create(int status);
void log_error_pthread_detach(int status);
void log_error_pthread_cancel(int status);
void log_error_pthread_join(int status);

void log_error_pthread_condattr_init(int status);
void log_error_pthread_condattr_destroy(int status);
void log_error_pthread_condattr_setclock(int status);

void log_error_pthread_cond_init(int status);
void log_error_pthread_cond_destroy(int status);
void log_error_pthread_cond_wait(int status);
void log_error_pthread_cond_timedwait(int status);
void log_error_pthread_cond_signal(int status);
void log_error_pthread_cond_broadcast(int status);

void log_error_sigemptyset(void);
void log_error_sigaddset(void);
void log_error_pthread_sigmask(int status);
void log_error_sigaction(void);

void log_error_clock_gettime(void);

void *list_remove_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation);
int list_add_unless_any(t_list *list, void *data, bool (*condition)(void *, void*), void *comparation);
void *list_find_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation);
void list_destroy_and_free_elements(t_list *list);
bool pointers_match(void * ptr_1, void *ptr_2);

int shared_list_init(t_Shared_List *shared_list);
int shared_list_destroy(t_Shared_List *shared_list);

int cancel_and_join_pthread(pthread_t *thread);
int wrapper_pthread_cancel(pthread_t *thread);
int wrapper_pthread_join(t_Bool_Join_Thread *join_thread);
void error_pthread(void);

#endif // UTILS_MODULE_H