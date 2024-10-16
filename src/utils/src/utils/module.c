/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "module.h"

t_log_level LOG_LEVEL = LOG_LEVEL_TRACE;

t_log *MINIMAL_LOGGER = NULL;
char *MINIMAL_LOG_PATHNAME = "minimal.log";

t_log *SOCKET_LOGGER = NULL;
char *SOCKET_LOG_PATHNAME = "socket.log";

t_log *SERIALIZE_LOGGER = NULL;
char *SERIALIZE_LOG_PATHNAME = "serialize.log";

void *signal_manager(pthread_t *thread_to_cancel) {
	int status;

	sigset_t set_SIGINT, set_rest;

	if(sigemptyset(&set_SIGINT)) {
		perror("sigemptyset");
		goto cancel;
	}

	if(sigaddset(&set_SIGINT, SIGINT)) {
		perror("sigaddset");
		goto cancel;
	}

	/*
	if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
		fprintf(stderr, "pthread_sigmask: %s\n", strerror(status));
		goto cancel;
	}
	*/

	if(sigfillset(&set_rest)) {
		perror("sigfillset");
		goto cancel;
	}

	if(sigdelset(&set_rest, SIGINT)) {
		perror("sigdelset");
		goto cancel;
	}

	if((status = pthread_sigmask(SIG_UNBLOCK, &set_rest, NULL))) {
		fprintf(stderr, "pthread_sigmask: %s\n", strerror(status));
		goto cancel;
	}

	siginfo_t info;
	int signo;
	if((signo = sigwaitinfo(&set_SIGINT, &info)) == -1) {
		perror("sigwaitinfo");
		goto cancel;
	}

	fprintf(stderr, "Signal SIGINT recibida\n");

	cancel:
		if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
			fprintf(stderr, "pthread_sigmask: %s\n", strerror(status));
		}

		if((status = pthread_sigmask(SIG_BLOCK, &set_rest, NULL))) {
			fprintf(stderr, "pthread_sigmask: %s\n", strerror(status));
		}

		if((status = pthread_cancel(*thread_to_cancel))) {
			fprintf(stderr, "pthread_cancel: %s\n", strerror(status));
		}

		pthread_exit(NULL);
}

int initialize_configs(char *pathname) {
	MODULE_CONFIG = config_create(pathname);
	if(MODULE_CONFIG == NULL) {
		fprintf(stderr, "%s: No se pudo abrir el archivo de configuracion\n", pathname);
        goto error;
	}

	if(read_module_config(MODULE_CONFIG)) {
		fprintf(stderr, "%s: El archivo de configuración no se pudo leer correctamente\n", pathname);
		goto error_config;
	}

	return 0;

	error_config:
		config_destroy(MODULE_CONFIG);
	error:
		return -1;
}

void finish_configs(void) {
	if(MODULE_CONFIG != NULL)
		config_destroy(MODULE_CONFIG);
}

bool config_has_properties(t_config *config, ...) {
    va_list args;
    va_start(args, config);

	bool result = true;
    
    char *property;
    while((property = va_arg(args, char *)) != NULL) {
        if(!config_has_property(config, property)) {
            fprintf(stderr, "%s: El archivo de configuración no contiene la clave %s\n", config->path, property);
            va_end(args);
            result = false;
        }
    }

    va_end(args);
    return result;
}

int initialize_loggers(void) {
	if(initialize_logger(&MINIMAL_LOGGER, MINIMAL_LOG_PATHNAME, "Minimal"))
		goto error;
	if(initialize_logger(&MODULE_LOGGER, MODULE_LOG_PATHNAME, MODULE_NAME))
		goto error_minimal_logger;
	if(initialize_logger(&SOCKET_LOGGER, SOCKET_LOG_PATHNAME, "Socket"))
		goto error_module_logger;
	if(initialize_logger(&SERIALIZE_LOGGER, SERIALIZE_LOG_PATHNAME, "Serialize"))
		goto error_socket_logger;

	return 0;

	error_socket_logger:
		finish_logger(&SOCKET_LOGGER);
	error_module_logger:
		finish_logger(&MODULE_LOGGER);
	error_minimal_logger:
		finish_logger(&MINIMAL_LOGGER);
	error:
		return -1;
}

int finish_loggers(void) {
	int retval = 0;

	if(finish_logger(&SERIALIZE_LOGGER))
		retval = -1;
	if(finish_logger(&SOCKET_LOGGER))
		retval = -1;
	if(finish_logger(&MODULE_LOGGER))
		retval = -1;
	if(finish_logger(&MINIMAL_LOGGER))
		retval = -1;

	return retval;
}

int initialize_logger(t_log **logger, char *pathname, char *module_name) {
	if(logger == NULL || pathname == NULL || module_name == NULL) {
		return -1;
	}

	if((*logger = log_create(pathname, module_name, true, LOG_LEVEL)) == NULL) {
		fprintf(stderr, "%s: No se pudo crear el logger\n", pathname);
		return -1;
	}

	return 0;
}

int finish_logger(t_log **logger) {
	if(logger == NULL) {
		return -1;
	}

	if(*logger != NULL) {
		log_destroy(*logger);
		*logger = NULL;
	}

	return 0;
}

void log_error_close(void) {
	log_error(MODULE_LOGGER, "close: %s", strerror(errno));
}

void log_error_fclose(void) {
	log_error(MODULE_LOGGER, "fclose: %s", strerror(errno));
}

void log_error_sem_init(void) {
	log_error(MODULE_LOGGER, "sem_init: %s", strerror(errno));
}

void log_error_sem_destroy(void) {
	log_error(MODULE_LOGGER, "sem_destroy: %s", strerror(errno));
}

void log_error_sem_wait(void) {
	log_error(MODULE_LOGGER, "sem_wait: %s", strerror(errno));
}

void log_error_sem_post(void) {
	log_error(MODULE_LOGGER, "sem_post: %s", strerror(errno));
}

void log_error_pthread_mutex_init(int status) {
	log_error(MODULE_LOGGER, "pthread_mutex_init: %s", strerror(status));
}

void log_error_pthread_mutex_destroy(int status) {
	log_error(MODULE_LOGGER, "pthread_mutex_destroy: %s", strerror(status));
}

void log_error_pthread_mutex_lock(int status) {
	log_error(MODULE_LOGGER, "pthread_mutex_lock: %s", strerror(status));
}

void log_error_pthread_mutex_unlock(int status) {
	log_error(MODULE_LOGGER, "pthread_mutex_unlock: %s", strerror(status));
}

void log_error_pthread_create(int status) {
	log_error(MODULE_LOGGER, "pthread_create: %s", strerror(status));
}

void log_error_pthread_detach(int status) {
	log_error(MODULE_LOGGER, "pthread_detach: %s", strerror(status));
}

void log_error_pthread_cancel(int status) {
	log_error(MODULE_LOGGER, "pthread_cancel: %s", strerror(status));
}

void log_error_pthread_join(int status) {
	log_error(MODULE_LOGGER, "pthread_join: %s", strerror(status));
}

void log_error_pthread_condattr_init(int status) {
	log_error(MODULE_LOGGER, "pthread_condattr_init: %s", strerror(status));
}

void log_error_pthread_condattr_destroy(int status) {
	log_error(MODULE_LOGGER, "pthread_condattr_destroy: %s", strerror(status));
}

void log_error_pthread_condattr_setclock(int status) {
	log_error(MODULE_LOGGER, "pthread_condattr_setclock: %s", strerror(status));
}

void log_error_pthread_cond_init(int status) {
	log_error(MODULE_LOGGER, "pthread_cond_init: %s", strerror(status));
}

void log_error_pthread_cond_destroy(int status) {
	log_error(MODULE_LOGGER, "pthread_cond_destroy: %s", strerror(status));
}

void log_error_pthread_cond_wait(int status) {
	log_error(MODULE_LOGGER, "pthread_cond_wait: %s", strerror(status));
}

void log_error_pthread_cond_timedwait(int status) {
	log_error(MODULE_LOGGER, "pthread_cond_timedwait: %s", strerror(status));
}

void log_error_pthread_cond_signal(int status) {
	log_error(MODULE_LOGGER, "pthread_cond_signal: %s", strerror(status));
}

void log_error_pthread_cond_broadcast(int status) {
	log_error(MODULE_LOGGER, "pthread_cond_broadcast: %s", strerror(status));
}

void log_error_sigemptyset(void) {
	log_error(MODULE_LOGGER, "sigemptyset: %s", strerror(errno));
}

void log_error_sigaddset(void) {
	log_error(MODULE_LOGGER, "sigaddset: %s", strerror(errno));
}

void log_error_pthread_sigmask(int status) {
	log_error(MODULE_LOGGER, "pthread_sigmask: %s", strerror(status));
}

void log_error_sigaction(void) {
	log_error(MODULE_LOGGER, "sigaction: %s", strerror(errno));
}

void log_error_clock_gettime(void) {
	log_error(MODULE_LOGGER, "clock_gettime: %s", strerror(errno));
}

int resource_sync_init(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	int status;

	resource_sync->ongoing_count = 0;

	if((status = pthread_mutex_init(&(resource_sync->mutex_resource), NULL))) {
		log_error_pthread_mutex_init(status);
		goto error;
	}

	resource_sync->drain_requests_count = 0;

	if((status = pthread_cond_init(&(resource_sync->cond_drain_requests), NULL))) {
		log_error_pthread_cond_init(status);
		goto error_mutex_resource;
	}

	resource_sync->drain_go_requests_count = 0;

	if(pthread_cond_init(&(resource_sync->cond_go_requests), NULL)) {
		log_error_pthread_cond_init(status);
		goto error_cond_drain_requests;
	}

	return 0;

	error_cond_drain_requests:
		if((status = pthread_cond_destroy(&(resource_sync->cond_drain_requests)))) {
			log_error_pthread_cond_destroy(status);
		}
	error_mutex_resource:
		if((status = pthread_mutex_destroy(&(resource_sync->mutex_resource)))) {
			log_error_pthread_mutex_destroy(status);
		}
	error:
		return -1;
}

int resource_sync_destroy(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(resource_sync == NULL) {
		errno = EINVAL;
		return -1;
	}

	int retval = 0, status;

	if((status = pthread_cond_destroy(&(resource_sync->cond_go_requests)))) {
		log_error_pthread_cond_destroy(status);
		retval = -1;
	}
	if((status = pthread_cond_destroy(&(resource_sync->cond_drain_requests)))) {
		log_error_pthread_cond_destroy(status);
		retval = -1;
	}
	if((status = pthread_mutex_destroy(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	return retval;
}

int wait_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	 // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	int status;

    if(wait_ongoing_locking(resource_sync)) {
		goto error;
	}

	if((status = pthread_mutex_unlock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_unlock(status);
		goto error;
	}

	return 0;

	error:
		return -1;
}

int signal_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	// TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	int status;

	if((status = pthread_mutex_lock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_lock(status);
		goto error;
	}

		resource_sync->drain_requests_count--;

		// TODO: Acá se podría agregar un if para hacer el signal sólo si el semáforo efectivamente quedó en 0
		if(resource_sync->drain_requests_count == 0) {
			if((status = pthread_cond_broadcast(&(resource_sync->cond_drain_requests)))) {
				log_error_pthread_cond_broadcast(status);
				goto error;
			}
		}

	if((status = pthread_mutex_unlock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_unlock(status);
		goto error;
	}

	return 0;

	error:
		return -1;
}

int wait_ongoing_locking(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	// TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	int status;

	if((status = pthread_mutex_lock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_lock(status);
		goto error;
	}

		resource_sync->drain_requests_count++;

		while(1) {

			if((resource_sync->ongoing_count) == 0)
				break;

			if((status = pthread_cond_wait(&(resource_sync->cond_go_requests), &(resource_sync->mutex_resource)))) {
				log_error_pthread_cond_wait(status);
				goto error;
			}
		}

	return 0;

	error:
		return -1;
}

int signal_ongoing_unlocking(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	// TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	int status;

	if((status = pthread_mutex_unlock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_unlock(status);
		goto error;
	}

	if(signal_ongoing(resource_sync)) {
		goto error;
	}

	return 0;

	error:
		return -1;
}

int wait_draining_requests(t_Drain_Ongoing_Resource_Sync *resource_sync) { // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	int status;

	if((status = pthread_mutex_lock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_lock(status);
		goto error;
	}

		while(1) {
			if((resource_sync->drain_requests_count) == 0)
				break;

			if((status = pthread_cond_wait(&(resource_sync->cond_drain_requests), &(resource_sync->mutex_resource)))) {
				log_error_pthread_cond_wait(status);
				goto error;
			}
		}

		resource_sync->ongoing_count++;

	if((status = pthread_mutex_unlock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_unlock(status);
		goto error;
	}

	return 0;

	error:
		return -1;
}

int signal_draining_requests(t_Drain_Ongoing_Resource_Sync *resource_sync) { // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	int status;

	if((status = pthread_mutex_lock(&(resource_sync->mutex_resource)))) {
		log_error_pthread_mutex_lock(status);
		goto error;
	}

		resource_sync->ongoing_count--;

		// Acá se podría agregar un if para hacer el signal sólo si el semáforo efectivamente quedó en 0
		if((status = pthread_cond_signal(&(resource_sync->cond_go_requests)))) {// podría ser un broadcast en lugar de un wait si hay más de un comando de consola esperando
			log_error_pthread_cond_signal(status);
			goto error;
		}

	if((status = pthread_mutex_unlock(&(resource_sync->mutex_resource)))) {
		goto error;
	}

	return 0;

	error:
		return -1;
}

void *list_remove_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation) {
	if(list == NULL || condition == NULL || comparation == NULL) {
		return NULL;
	}

	t_link_element **indirect = &(list->head);
	while (*indirect != NULL) {
		if(condition(((*indirect)->data), comparation)) {
			t_link_element *removed_element = *indirect;

			*indirect = (*indirect)->next;
			list->elements_count--;

			void *data = removed_element->data;
			free(removed_element);
			return data;
		}
		indirect = &((*indirect)->next);
	}
	return NULL;
}

int list_add_unless_any(t_list *list, void *data, bool (*condition)(void *, void*), void *comparation) {
	if(list == NULL || condition == NULL || comparation == NULL)
		return -1;

    t_link_element **indirect = &(list->head);
    while (*indirect != NULL) {
        if(condition((*indirect)->data, comparation))
            return -1;
        indirect = &((*indirect)->next);
    }

    t_link_element *new_element = (t_link_element *) malloc(sizeof(t_link_element));
    if(new_element == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para un nuevo elemento de la lista", sizeof(t_link_element));
        return -1;
    }

    new_element->data = data;
    new_element->next = NULL;

    *indirect = new_element;
    list->elements_count++;

    return 0;
}

void *list_find_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation) {
	if(list == NULL || condition == NULL || comparation == NULL)
		return NULL;

	t_link_element **indirect = &(list->head);
	while (*indirect != NULL) {
		if(condition(((*indirect)->data), comparation)) {
			return (*indirect)->data;
		}
		indirect = &((*indirect)->next);
	}
	return NULL;
}

bool pointers_match(void *ptr_1, void *ptr_2) {
	return ptr_1 == ptr_2;
}

int shared_list_init(t_Shared_List *shared_list) {
	int retval = 0, status;

	if((status = pthread_mutex_init(&(shared_list->mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(shared_list->mutex));

	shared_list->list = list_create();
	if(shared_list->list == NULL) {
		log_error(MODULE_LOGGER, "shared_list_init: No se pudo crear la lista");
		retval = -1;
		goto cleanup;
	}

	cleanup:
		pthread_cleanup_pop(status); // shared_list->mutex
	ret:
		return retval;
}

int shared_list_destroy(t_Shared_List *shared_list) {
	int retval = 0, status;

	list_destroy(shared_list->list);

	if((status = pthread_mutex_destroy(&(shared_list->mutex)))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	return retval;
}

int cancel_and_join_pthread(pthread_t *thread) {
	int status;

	if(thread == NULL) {
		errno = EINVAL;
		return -1;
	}

	if((status = pthread_cancel(*thread))) {
		log_error_pthread_cancel(status);
		return -1;
	}

	if((status = pthread_join(*thread, NULL))) {
		log_error_pthread_join(status);
		return -1;
	}

	return 0;
}

void error_pthread(void) {
	pthread_kill(pthread_self(), SIGINT);
	pthread_exit(NULL);
}