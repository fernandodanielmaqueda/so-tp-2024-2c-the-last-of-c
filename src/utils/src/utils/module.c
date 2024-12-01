/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "module.h"

pthread_t THREAD_SIGNAL_MANAGER;

char *MODULE_NAME;

t_config *MODULE_CONFIG = NULL;
char *MODULE_CONFIG_PATHNAME;

t_log_level LOG_LEVEL = LOG_LEVEL_TRACE;

pthread_mutex_t MUTEX_LOGGERS;
t_Logger MODULE_LOGGER;
t_Logger MINIMAL_LOGGER;
t_Logger SOCKET_LOGGER;
t_Logger SERIALIZE_LOGGER;

void *signal_manager(pthread_t *thread_to_cancel) {
	int status;

	sigset_t set_SIGINT, set_rest;

	if(sigemptyset(&set_SIGINT)) {
		report_error_sigemptyset();
		goto cancel;
	}

	if(sigaddset(&set_SIGINT, SIGINT)) {
		report_error_sigaddset();
		goto cancel;
	}

	/*
	if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
		report_error_pthread_sigmask(status);
		goto cancel;
	}
	*/

	if(sigfillset(&set_rest)) {
		report_error_sigfillset();
		goto cancel;
	}

	if(sigdelset(&set_rest, SIGINT)) {
		report_error_sigdelset();
		goto cancel;
	}

	if((status = pthread_sigmask(SIG_UNBLOCK, &set_rest, NULL))) {
		report_error_pthread_sigmask(status);
		goto cancel;
	}

	siginfo_t info;
	int signo;
	while((signo = sigwaitinfo(&set_SIGINT, &info)) == -1) {
		report_error_sigwaitinfo();
		//goto cancel;
	}

	fprintf(stderr, "\nSIGINT recibida\n");

	cancel:
		if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
			report_error_pthread_sigmask(status);
		}

		if((status = pthread_sigmask(SIG_BLOCK, &set_rest, NULL))) {
			report_error_pthread_sigmask(status);
		}

		if((status = pthread_cancel(*thread_to_cancel))) {
			report_error_pthread_cancel(status);
		}

		pthread_exit(NULL);
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

int logger_init(t_Logger *logger, bool enabled, char *pathname, char *name, bool is_active_console, t_log_level log_level) {
	int retval = 0, status;
	if(logger == NULL || pathname == NULL || name == NULL) {
		fprintf(stderr, "logger_init: %s\n", strerror(EINVAL));
		return -1;
	}

	logger->enabled = enabled;

	if(((logger->log) = log_create(pathname, name, is_active_console, log_level)) == NULL) {
		fprintf(stderr, "%s: No se pudo crear el log\n", pathname);
		return -1;
	}
	pthread_cleanup_push((void (*)(void *)) log_destroy, logger->log);

	cleanup_log:
	pthread_cleanup_pop(retval); // log

	return retval;
}

int logger_destroy(t_Logger *logger) {
	int retval = 0, status;
	if(logger == NULL) {
		fprintf(stderr, "logger_destroy: %s\n", strerror(EINVAL));
		return -1;
	}

	log_destroy(logger->log);

	return retval;
}

void report_error_close(void) {
	fprintf(stderr, "close: %s\n", strerror(errno));
}

void report_error_fclose(void) {
	fprintf(stderr, "fclose: %s\n", strerror(errno));
}

void report_error_sem_init(void) {
	fprintf(stderr, "sem_init: %s\n", strerror(errno));
}

void report_error_sem_destroy(void) {
	fprintf(stderr, "sem_destroy: %s\n", strerror(errno));
}

void report_error_sem_wait(void) {
	fprintf(stderr, "sem_wait: %s\n", strerror(errno));
}

void report_error_sem_post(void) {
	fprintf(stderr, "sem_post: %s\n", strerror(errno));
}

void report_error_pthread_mutex_init(int status) {
	fprintf(stderr, "pthread_mutex_init: %s\n", strerror(status));
}

void report_error_pthread_mutex_destroy(int status) {
	fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(status));
}

void report_error_pthread_mutex_lock(int status) {
	fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(status));
}

void report_error_pthread_mutex_unlock(int status) {
	fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(status));
}

void report_error_pthread_rwlock_init(int status) {
	fprintf(stderr, "pthread_rwlock_init: %s\n", strerror(status));
}

void report_error_pthread_rwlock_destroy(int status) {
	fprintf(stderr, "pthread_rwlock_destroy: %s\n", strerror(status));
}

void report_error_pthread_rwlock_wrlock(int status) {
	fprintf(stderr, "pthread_rwlock_wrlock: %s\n", strerror(status));
}

void report_error_pthread_rwlock_rdlock(int status) {
	fprintf(stderr, "pthread_rwlock_rdlock: %s\n", strerror(status));
}

void report_error_pthread_rwlock_unlock(int status) {
	fprintf(stderr, "pthread_rwlock_unlock: %s\n", strerror(status));
}

void report_error_pthread_create(int status) {
	fprintf(stderr, "pthread_create: %s\n", strerror(status));
}

void report_error_pthread_detach(int status) {
	fprintf(stderr, "pthread_detach: %s\n", strerror(status));
}

void report_error_pthread_cancel(int status) {
	fprintf(stderr, "pthread_cancel: %s\n", strerror(status));
}

void report_error_pthread_join(int status) {
	fprintf(stderr, "pthread_join: %s\n", strerror(status));
}

void report_error_pthread_condattr_init(int status) {
	fprintf(stderr, "pthread_condattr_init: %s\n", strerror(status));
}

void report_error_pthread_condattr_destroy(int status) {
	fprintf(stderr, "pthread_condattr_destroy: %s\n", strerror(status));
}

void report_error_pthread_condattr_setclock(int status) {
	fprintf(stderr, "pthread_condattr_setclock: %s\n", strerror(status));
}

void report_error_pthread_cond_init(int status) {
	fprintf(stderr, "pthread_cond_init: %s\n", strerror(status));
}

void report_error_pthread_cond_destroy(int status) {
	fprintf(stderr, "pthread_cond_destroy: %s\n", strerror(status));
}

void report_error_pthread_cond_wait(int status) {
	fprintf(stderr, "pthread_cond_wait: %s\n", strerror(status));
}

void report_error_pthread_cond_timedwait(int status) {
	fprintf(stderr, "pthread_cond_timedwait: %s\n", strerror(status));
}

void report_error_pthread_cond_signal(int status) {
	fprintf(stderr, "pthread_cond_signal: %s\n", strerror(status));
}

void report_error_pthread_cond_broadcast(int status) {
	fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(status));
}

void report_error_sigemptyset(void) {
	fprintf(stderr, "sigemptyset: %s\n", strerror(errno));
}

void report_error_sigfillset(void) {
	fprintf(stderr, "sigfillset: %s\n", strerror(errno));
}

void report_error_sigaddset(void) {
	fprintf(stderr, "sigaddset: %s\n", strerror(errno));
}

void report_error_sigdelset(void) {
	fprintf(stderr, "sigdelset: %s\n", strerror(errno));
}

void report_error_pthread_sigmask(int status) {
	fprintf(stderr, "pthread_sigmask: %s\n", strerror(status));
}

void report_error_sigaction(void) {
	fprintf(stderr, "sigaction: %s\n", strerror(errno));
}

void report_error_sigwaitinfo(void) {
	fprintf(stderr, "sigwaitinfo: %s\n", strerror(errno));
}

void report_error_clock_gettime(void) {
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
        log_error_r(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para un nuevo elemento de la lista", sizeof(t_link_element));
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
		report_error_pthread_mutex_init(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(shared_list->mutex));

	shared_list->list = list_create();
	if(shared_list->list == NULL) {
		log_error_r(MODULE_LOGGER, "shared_list_init: No se pudo crear la lista");
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
		report_error_pthread_mutex_destroy(status);
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
		report_error_pthread_cancel(status);
		return -1;
	}

	if((status = pthread_join(*thread, NULL))) {
		report_error_pthread_join(status);
		return -1;
	}

	return 0;
}

int wrapper_pthread_cancel(pthread_t *thread) {
	int status;

	if((status = pthread_cancel(*thread))) {
		report_error_pthread_cancel(status);
		return -1;
	}

	return 0;
}

int wrapper_pthread_join(pthread_t *thread) {
	int status;

	if((status = pthread_join(*thread, NULL))) {
		report_error_pthread_join(status);
		return -1;
	}

	return 0;
}

void exit_sigint(void) {
	pthread_kill(THREAD_SIGNAL_MANAGER, SIGINT); // Envia señal CTRL + C
	pthread_exit(NULL);
}