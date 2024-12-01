/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "module.h"

pthread_t THREAD_SIGNAL_MANAGER;

char *MODULE_NAME;

t_config *MODULE_CONFIG = NULL;
char *MODULE_CONFIG_PATHNAME;

t_log_level LOG_LEVEL = LOG_LEVEL_TRACE;

t_log *MODULE_LOGGER = NULL;
char *MODULE_LOG_PATHNAME;
pthread_mutex_t MUTEX_MODULE_LOGGER;

t_log *MINIMAL_LOGGER = NULL;
char *MINIMAL_LOG_PATHNAME = "minimal.log";
pthread_mutex_t MUTEX_MINIMAL_LOGGER;

t_log *SOCKET_LOGGER = NULL;
char *SOCKET_LOG_PATHNAME = "socket.log";
pthread_mutex_t MUTEX_SOCKET_LOGGER;

t_log *SERIALIZE_LOGGER = NULL;
char *SERIALIZE_LOG_PATHNAME = "serialize.log";
pthread_mutex_t MUTEX_SERIALIZE_LOGGER;

void *signal_manager(pthread_t *thread_to_cancel) {
	int status;

	sigset_t set_SIGINT, set_rest;

	if(sigemptyset(&set_SIGINT)) {
		log_error_sigemptyset();
		goto cancel;
	}

	if(sigaddset(&set_SIGINT, SIGINT)) {
		log_error_sigaddset();
		goto cancel;
	}

	/*
	if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
		log_error_pthread_sigmask(status);
		goto cancel;
	}
	*/

	if(sigfillset(&set_rest)) {
		log_error_sigfillset();
		goto cancel;
	}

	if(sigdelset(&set_rest, SIGINT)) {
		log_error_sigdelset();
		goto cancel;
	}

	if((status = pthread_sigmask(SIG_UNBLOCK, &set_rest, NULL))) {
		log_error_pthread_sigmask(status);
		goto cancel;
	}

	siginfo_t info;
	int signo;
	while((signo = sigwaitinfo(&set_SIGINT, &info)) == -1) {
		log_error_sigwaitinfo();
		//goto cancel;
	}

	fprintf(stderr, "\nSIGINT recibida\n");

	cancel:
		if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
			log_error_pthread_sigmask(status);
		}

		if((status = pthread_sigmask(SIG_BLOCK, &set_rest, NULL))) {
			log_error_pthread_sigmask(status);
		}

		if((status = pthread_cancel(*thread_to_cancel))) {
			log_error_pthread_cancel(status);
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
	fprintf(stderr, "close: %s\n", strerror(errno));
}

void log_error_fclose(void) {
	fprintf(stderr, "fclose: %s\n", strerror(errno));
}

void log_error_sem_init(void) {
	fprintf(stderr, "sem_init: %s\n", strerror(errno));
}

void log_error_sem_destroy(void) {
	fprintf(stderr, "sem_destroy: %s\n", strerror(errno));
}

void log_error_sem_wait(void) {
	fprintf(stderr, "sem_wait: %s\n", strerror(errno));
}

void log_error_sem_post(void) {
	fprintf(stderr, "sem_post: %s\n", strerror(errno));
}

void log_error_pthread_mutex_init(int status) {
	fprintf(stderr, "pthread_mutex_init: %s\n", strerror(status));
}

void log_error_pthread_mutex_destroy(int status) {
	fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(status));
}

void log_error_pthread_mutex_lock(int status) {
	fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(status));
}

void log_error_pthread_mutex_unlock(int status) {
	fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(status));
}

void log_error_pthread_rwlock_init(int status) {
	fprintf(stderr, "pthread_rwlock_init: %s\n", strerror(status));
}

void log_error_pthread_rwlock_destroy(int status) {
	fprintf(stderr, "pthread_rwlock_destroy: %s\n", strerror(status));
}

void log_error_pthread_rwlock_wrlock(int status) {
	fprintf(stderr, "pthread_rwlock_wrlock: %s\n", strerror(status));
}

void log_error_pthread_rwlock_rdlock(int status) {
	fprintf(stderr, "pthread_rwlock_rdlock: %s\n", strerror(status));
}

void log_error_pthread_rwlock_unlock(int status) {
	fprintf(stderr, "pthread_rwlock_unlock: %s\n", strerror(status));
}

void log_error_pthread_create(int status) {
	fprintf(stderr, "pthread_create: %s\n", strerror(status));
}

void log_error_pthread_detach(int status) {
	fprintf(stderr, "pthread_detach: %s\n", strerror(status));
}

void log_error_pthread_cancel(int status) {
	fprintf(stderr, "pthread_cancel: %s\n", strerror(status));
}

void log_error_pthread_join(int status) {
	fprintf(stderr, "pthread_join: %s\n", strerror(status));
}

void log_error_pthread_condattr_init(int status) {
	fprintf(stderr, "pthread_condattr_init: %s\n", strerror(status));
}

void log_error_pthread_condattr_destroy(int status) {
	fprintf(stderr, "pthread_condattr_destroy: %s\n", strerror(status));
}

void log_error_pthread_condattr_setclock(int status) {
	fprintf(stderr, "pthread_condattr_setclock: %s\n", strerror(status));
}

void log_error_pthread_cond_init(int status) {
	fprintf(stderr, "pthread_cond_init: %s\n", strerror(status));
}

void log_error_pthread_cond_destroy(int status) {
	fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(status));
}

void log_error_pthread_cond_wait(int status) {
	fprintf(stderr, "pthread_cond_wait: %s\n", strerror(status));
}

void log_error_pthread_cond_timedwait(int status) {
	fprintf(stderr, "pthread_cond_timedwait: %s\n", strerror(status));
}

void log_error_pthread_cond_signal(int status) {
	fprintf(stderr, "pthread_cond_signal: %s\n", strerror(status));
}

void log_error_pthread_cond_broadcast(int status) {
	fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(status));
}

void log_error_sigemptyset(void) {
	fprintf(stderr, "sigemptyset: %s\n", strerror(errno));
}

void log_error_sigfillset(void) {
	fprintf(stderr, "sigfillset: %s\n", strerror(errno));
}

void log_error_sigaddset(void) {
	fprintf(stderr, "sigaddset: %s\n", strerror(errno));
}

void log_error_sigdelset(void) {
	fprintf(stderr, "sigdelset: %s\n", strerror(errno));
}

void log_error_pthread_sigmask(int status) {
	fprintf(stderr, "pthread_sigmask: %s\n", strerror(status));
}

void log_error_sigaction(void) {
	fprintf(stderr, "sigaction: %s\n", strerror(errno));
}

void log_error_sigwaitinfo(void) {
	fprintf(stderr, "sigwaitinfo: %s\n", strerror(errno));
}

void log_error_clock_gettime(void) {
	fprintf(stderr, "clock_gettime: %s\n", strerror(errno));
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

	t_link_element *current = list->head;
	while(current != NULL) {
		if(condition((current->data), comparation)) {
			return current->data;
		}
		current = current->next;
	}
	return NULL;
}

void list_destroy_and_free_elements(t_list *list) {
	if(list == NULL)
		return;

	t_link_element *next;

	t_link_element *element = list->head;
	while(element != NULL) {
		next = element->next;
		free(element->data);
		free(element);
		element = next;
	}

	free(list);
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

void conditional_cleanup(t_Conditional_Cleanup *this) {
	if(this == NULL || this->condition == NULL || this->function == NULL)
		return;

	if((*(this->condition)) ^ (this->negate_condition)) {
		(this->function)(this->argument);
	}
}

int cancel_and_join_pthread(pthread_t *thread) {
	int status;

	if(thread == NULL) {
		fprintf(stderr, "cancel_and_join_pthread: %s\n", strerror(EINVAL));
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

int wrapper_pthread_cancel(pthread_t *thread) {
	int status;

	if((status = pthread_cancel(*thread))) {
		log_error_pthread_cancel(status);
		return -1;
	}

	return 0;
}

int wrapper_pthread_join(pthread_t *thread) {
	int status;

	if((status = pthread_join(*thread, NULL))) {
		log_error_pthread_join(status);
		return -1;
	}

	return 0;
}

void exit_sigint(void) {
	pthread_kill(THREAD_SIGNAL_MANAGER, SIGINT); // Envia señal CTRL + C
	pthread_exit(NULL);
}