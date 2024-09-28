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

int initialize_configs(char *pathname) {
	MODULE_CONFIG = config_create(pathname);
	if(MODULE_CONFIG == NULL) {
		fprintf(stderr, "%s: No se pudo abrir el archivo de configuracion\n", pathname);
        return -1;
	}

	if(read_module_config(MODULE_CONFIG)) {
		fprintf(stderr, "%s: El archivo de configuración no se pudo leer correctamente\n", pathname);
		config_destroy(MODULE_CONFIG);
		return -1;
	}

	return 0;
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
		goto error;
	if(initialize_logger(&SOCKET_LOGGER, SOCKET_LOG_PATHNAME, "Socket"))
		goto error;
	if(initialize_logger(&SERIALIZE_LOGGER, SERIALIZE_LOG_PATHNAME, "Serialize"))
		goto error;

	return 0;

	error:
		finish_loggers();
		return -1;
}

int finish_loggers(void) {
	int status = 0;

	if(finish_logger(&SERIALIZE_LOGGER))
		status = -1;
	if(finish_logger(&SOCKET_LOGGER))
		status = -1;
	if(finish_logger(&MODULE_LOGGER))
		status = -1;
	if(finish_logger(&MINIMAL_LOGGER))
		status = -1;

	return status;
}

int initialize_logger(t_log **logger, char *pathname, char *module_name) {
	if(logger == NULL || pathname == NULL || module_name == NULL) {
		return -1;
	}

	*logger = log_create(pathname, module_name, true, LOG_LEVEL);
	if(*logger == NULL) {
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

int init_resource_sync(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	resource_sync->ongoing_count = 0;

	if(pthread_mutex_init(&(resource_sync->mutex_resource), NULL))
		goto error;
	
	resource_sync->drain_requests_count = 0;

	if(pthread_cond_init(&(resource_sync->cond_drain_requests), NULL))
		goto error_mutex_resource;
	
	resource_sync->drain_go_requests_count = 0;

	if(pthread_cond_init(&(resource_sync->cond_go_requests), NULL))
		goto error_cond_drain_requests;
	
	resource_sync->initialized = true;

	return 0;

	error_cond_drain_requests:
		pthread_cond_destroy(&(resource_sync->cond_drain_requests));
	error_mutex_resource:
		pthread_mutex_destroy(&(resource_sync->mutex_resource));
	error:
		return -1;
}

int destroy_resource_sync(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(resource_sync == NULL) {
		errno = EINVAL;
		return -1;
	}

	if(!resource_sync->initialized)
		return 0;

	int status = 0;

	resource_sync->initialized = false;

	if(pthread_cond_destroy(&(resource_sync->cond_go_requests)))
		status = -1;
	if(pthread_cond_destroy(&(resource_sync->cond_drain_requests)))
		status = -1;
	if(pthread_mutex_destroy(&(resource_sync->mutex_resource)))
		status = -1;

	return status;
}

int wait_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync) { // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

    if(wait_ongoing_locking(resource_sync))
		goto error;

	if(pthread_mutex_unlock(&(resource_sync->mutex_resource)))
		goto error;

	return 0;

	error:
		return -1;
}

int signal_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync) { // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		goto error;
	}

		resource_sync->drain_requests_count--;

		// TODO: Acá se podría agregar un if para hacer el signal sólo si el semáforo efectivamente quedó en 0
		if(resource_sync->drain_requests_count == 0) {
			if(pthread_cond_broadcast(&(resource_sync->cond_drain_requests)))
				goto error;
		}

	if(pthread_mutex_unlock(&(resource_sync->mutex_resource)))
		goto error;

	return 0;

	error:
		return -1;
}

int wait_ongoing_locking(t_Drain_Ongoing_Resource_Sync *resource_sync) { // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		goto error;
	}

		resource_sync->drain_requests_count++;

		while(1) {

			if((resource_sync->ongoing_count) == 0)
				break;

			if(pthread_cond_wait(&(resource_sync->cond_go_requests), &(resource_sync->mutex_resource)))
				goto error;
		}

	return 0;

	error:
		return -1;
}

int signal_ongoing_unlocking(t_Drain_Ongoing_Resource_Sync *resource_sync) { // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	if(pthread_mutex_unlock(&(resource_sync->mutex_resource)))
		goto error;

	if(signal_ongoing(resource_sync))
		goto error;

	return 0;

	error:
		return -1;
}

int wait_draining_requests(t_Drain_Ongoing_Resource_Sync *resource_sync) { // TODO
	if(resource_sync == NULL) {
		errno = EINVAL;
		goto error;
	}

	if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		goto error;
	}

		while(1) {
			if((resource_sync->drain_requests_count) == 0)
				break;

			if(pthread_cond_wait(&(resource_sync->cond_drain_requests), &(resource_sync->mutex_resource))) {
				goto error;
			}
		}

		resource_sync->ongoing_count++;
    
	if(pthread_mutex_unlock(&(resource_sync->mutex_resource))) {
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

	if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		goto error;
	}
	
		resource_sync->ongoing_count--;
		
		// Acá se podría agregar un if para hacer el signal sólo si el semáforo efectivamente quedó en 0
		if(pthread_cond_signal(&(resource_sync->cond_go_requests))) {// podría ser un broadcast en lugar de un wait si hay más de un comando de consola esperando
			goto error;
		}
	
	if(pthread_mutex_unlock(&(resource_sync->mutex_resource))) {
		goto error;
	}

	return 0;

	error:
		return -1;
}

void *list_remove_by_condition_with_comparation(t_list *list, bool (*condition)(void *, void *), void *comparation) {
	if(list == NULL || condition == NULL || comparation == NULL)
		return NULL;

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

bool pointers_match(void * ptr_1, void *ptr_2) {
	return ptr_1 == ptr_2;
}