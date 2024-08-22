/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_MODULE_H
#define UTILS_MODULE_H

#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include "commons/log.h"
#include "commons/config.h"

typedef struct t_Shared_List {
    t_list *list;
    pthread_mutex_t mutex;
} t_Shared_List;

typedef struct t_Drain_Ongoing_Resource_Sync {
	pthread_mutex_t mutex_resource;
	unsigned int drain_requests_count;
	pthread_cond_t cond_drain_requests;
	unsigned int ongoing_count;
	pthread_cond_t cond_ongoing;
} t_Drain_Ongoing_Resource_Sync;

extern char *MODULE_NAME;

extern t_log *MINIMAL_LOGGER;
extern char *MINIMAL_LOG_PATHNAME;

extern t_log *MODULE_LOGGER;
extern char *MODULE_LOG_PATHNAME;

extern t_log *SOCKET_LOGGER;
extern char *SOCKET_LOG_PATHNAME;

extern t_log *SERIALIZE_LOGGER;
extern char *SERIALIZE_LOG_PATHNAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

void initialize_loggers(void);
void finish_loggers(void);
void initialize_configs(char *pathname);
void finish_configs(void);
void read_module_config(t_config*);

void init_resource_sync(t_Drain_Ongoing_Resource_Sync *resource_sync);
void destroy_resource_sync(t_Drain_Ongoing_Resource_Sync *resource_sync);
void wait_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync);
void signal_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync);
void wait_ongoing_locking(t_Drain_Ongoing_Resource_Sync *resource_sync);
void signal_ongoing_unlocking(t_Drain_Ongoing_Resource_Sync *resource_sync);
void wait_draining_requests(t_Drain_Ongoing_Resource_Sync *resource_sync);
void signal_draining_requests(t_Drain_Ongoing_Resource_Sync *resource_sync);

void *list_remove_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation);
int list_add_unless_any(t_list *list, void *data, bool (*condition)(void *, void*), void *comparation);
void *list_find_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation);
bool pointers_match(void * ptr_1, void *ptr_2);

#endif // UTILS_MODULE_H