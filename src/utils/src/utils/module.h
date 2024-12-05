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

typedef struct t_Logger {
    bool enabled;
    t_log *log;
} t_Logger;

typedef struct t_Bool_Thread {
    pthread_t thread;
    bool running;
} t_Bool_Thread;

typedef struct t_Shared_List {
    t_list *list;
    pthread_mutex_t mutex;
} t_Shared_List;

typedef struct t_Conditional_Cleanup {
    bool *condition;
    bool negate_condition;
    void (*function)(void *);
    void *argument;
} t_Conditional_Cleanup;

extern pthread_t THREAD_SIGNAL_MANAGER;

extern char *MODULE_NAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

extern t_log_level LOG_LEVEL;

extern pthread_mutex_t MUTEX_LOGGERS;
extern t_Logger MODULE_LOGGER;
extern t_Logger MINIMAL_LOGGER;
extern t_Logger SOCKET_LOGGER;
extern t_Logger SERIALIZE_LOGGER;

#define MODULE_LOGGER_INIT_ENABLED true
#define MODULE_LOGGER_PATHNAME "module.log"
#define MODULE_LOGGER_NAME "Module"
#define MODULE_LOGGER_INIT_ACTIVE_CONSOLE true
#define MODULE_LOGGER_INIT_LOG_LEVEL LOG_LEVEL

#define MINIMAL_LOGGER_INIT_ENABLED true
#define MINIMAL_LOGGER_PATHNAME "minimal.log"
#define MINIMAL_LOGGER_NAME "Minimal"
#define MINIMAL_LOGGER_ACTIVE_CONSOLE true
#define MINIMAL_LOGGER_INIT_LOG_LEVEL LOG_LEVEL

#define SOCKET_LOGGER_INIT_ENABLED true
#define SOCKET_LOGGER_PATHNAME "socket.log"
#define SOCKET_LOGGER_NAME "Socket"
#define SOCKET_LOGGER_INIT_ACTIVE_CONSOLE true
#define SOCKET_LOGGER_INIT_LOG_LEVEL LOG_LEVEL

#define SERIALIZE_LOGGER_INIT_ENABLED true
#define SERIALIZE_LOGGER_PATHNAME "serialize.log"
#define SERIALIZE_LOGGER_NAME "Serialize"
#define SERIALIZE_LOGGER_INIT_ACTIVE_CONSOLE false
#define SERIALIZE_LOGGER_INIT_LOG_LEVEL LOG_LEVEL

#define PTHREAD_SETCANCELSTATE_DISABLE() \
    do {                         \
        int __old_cancel_state;  \
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &__old_cancel_state)

#define PTHREAD_SETCANCELSTATE_OLDSTATE()                          \
        pthread_setcancelstate(__old_cancel_state, NULL); \
    } while(0)

void *signal_manager(pthread_t *thread_to_cancel);

bool config_has_properties(t_config *config, ...);
extern int read_module_config(t_config *); // Se debe implementar en cada módulo

int logger_init(t_Logger *logger, bool enabled, char *pathname, char *name, bool is_active_console, t_log_level log_level);
int logger_destroy(t_Logger *logger);

#define log_trace_r(logger, format, ...) \
    do { \
        if ((logger)->enabled) { \
            pthread_mutex_lock(&MUTEX_LOGGERS); \
                log_trace((logger)->log, format, ##__VA_ARGS__); \
            pthread_mutex_unlock(&MUTEX_LOGGERS); \
        } \
    } while (0)

#define log_debug_r(logger, format, ...) \
    do { \
        if ((logger)->enabled) { \
            pthread_mutex_lock(&MUTEX_LOGGERS); \
                log_debug((logger)->log, format, ##__VA_ARGS__); \
            pthread_mutex_unlock(&MUTEX_LOGGERS); \
        } \
    } while (0)

#define log_info_r(logger, format, ...) \
    do { \
        if ((logger)->enabled) { \
            pthread_mutex_lock(&MUTEX_LOGGERS); \
                log_info((logger)->log, format, ##__VA_ARGS__); \
            pthread_mutex_unlock(&MUTEX_LOGGERS); \
        } \
    } while (0)

#define log_warning_r(logger, format, ...) \
    do { \
        if ((logger)->enabled) { \
            pthread_mutex_lock(&MUTEX_LOGGERS); \
                log_warning((logger)->log, format, ##__VA_ARGS__); \
            pthread_mutex_unlock(&MUTEX_LOGGERS); \
        } \
    } while (0)

#define log_error_r(logger, format, ...) \
    do { \
        if ((logger)->enabled) { \
            pthread_mutex_lock(&MUTEX_LOGGERS); \
                log_error((logger)->log, format, ##__VA_ARGS__); \
            pthread_mutex_unlock(&MUTEX_LOGGERS); \
        } \
    } while (0)

void report_error_strdup(void);

void report_error_close(void);

void report_error_fclose(void);

void report_error_sem_init(void);
void report_error_sem_destroy(void);
void report_error_sem_wait(void);
void report_error_sem_post(void);

void report_error_pthread_mutex_init(int status);
void report_error_pthread_mutex_destroy(int status);
void report_error_pthread_mutex_lock(int statusoid);
void report_error_pthread_mutex_unlock(int status);

void report_error_pthread_rwlock_init(int status);
void report_error_pthread_rwlock_destroy(int status);
void report_error_pthread_rwlock_wrlock(int status);
void report_error_pthread_rwlock_rdlock(int status);
void report_error_pthread_rwlock_unlock(int status);

void report_error_pthread_create(int status);
void report_error_pthread_detach(int status);
void report_error_pthread_cancel(int status);
void report_error_pthread_join(int status);

void report_error_pthread_condattr_init(int status);
void report_error_pthread_condattr_destroy(int status);
void report_error_pthread_condattr_setclock(int status);

void report_error_pthread_cond_init(int status);
void report_error_pthread_cond_destroy(int status);
void report_error_pthread_cond_wait(int status);
void report_error_pthread_cond_timedwait(int status);
void report_error_pthread_cond_signal(int status);
void report_error_pthread_cond_broadcast(int status);

void report_error_sigemptyset(void);
void report_error_sigfillset(void);
void report_error_sigaddset(void);
void report_error_sigdelset(void);
void report_error_pthread_sigmask(int status);
void report_error_sigaction(void);
void report_error_sigwaitinfo(void);

void report_error_clock_gettime(void);

void *list_remove_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation);
int list_add_unless_any(t_list *list, void *data, bool (*condition)(void *, void*), void *comparation);
void *list_find_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation);
void list_destroy_and_free_elements(t_list *list);
bool pointers_match(void * ptr_1, void *ptr_2);

int shared_list_init(t_Shared_List *shared_list);
int shared_list_destroy(t_Shared_List *shared_list);

void conditional_cleanup(t_Conditional_Cleanup *this);

int cancel_and_join_pthread(pthread_t *thread);
int wrapper_pthread_cancel(pthread_t *thread);
int wrapper_pthread_join(pthread_t *thread);
void exit_sigint(void);

#endif // UTILS_MODULE_H