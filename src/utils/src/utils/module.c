/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "module.h"

t_log_level LOG_LEVEL = LOG_LEVEL_TRACE;

t_log *MINIMAL_LOGGER;
char *MINIMAL_LOG_PATHNAME = "minimal.log";

t_log *SOCKET_LOGGER;
char *SOCKET_LOG_PATHNAME = "socket.log";

t_log *SERIALIZE_LOGGER;
char *SERIALIZE_LOG_PATHNAME = "serialize.log";

void initialize_loggers(void) {
	MINIMAL_LOGGER = log_create(MINIMAL_LOG_PATHNAME, "Minimal", true, LOG_LEVEL);
	MODULE_LOGGER = log_create(MODULE_LOG_PATHNAME, MODULE_NAME, true, LOG_LEVEL);
	SOCKET_LOGGER = log_create(SOCKET_LOG_PATHNAME, "Socket", true, LOG_LEVEL);
	SERIALIZE_LOGGER = log_create(SERIALIZE_LOG_PATHNAME, "Serialize", true, LOG_LEVEL);
}

void finish_loggers(void) {
	log_destroy(MINIMAL_LOGGER);
	log_destroy(MODULE_LOGGER);
	log_destroy(SOCKET_LOGGER);
	log_destroy(SERIALIZE_LOGGER);
}

void initialize_configs(char *pathname) {
	MODULE_CONFIG = config_create(pathname);

	if(MODULE_CONFIG == NULL) {
		fprintf(stderr, "%s: No se pudo abrir el archivo de configuracion", pathname);
        exit(EXIT_FAILURE);
	}

	read_module_config(MODULE_CONFIG);
}

void finish_configs(void) {
	config_destroy(MODULE_CONFIG);
}

bool config_has_properties(t_config *config, ...) {
    va_list args;
    va_start(args, config);
    
    char *property;
    while((property = va_arg(args, char *)) != NULL) {
        if(!config_has_property(config, property)) {
            fprintf(stderr, "%s: El archivo de configuración no contiene la clave %s", config->path, property);
            va_end(args);
            return 0;
        }
    }

    va_end(args);
    return -1;
}

int init_resource_sync(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(pthread_mutex_init(&(resource_sync->mutex_resource), NULL))
		return -1;
	
	resource_sync->drain_requests_count = 0;
	if(pthread_cond_init(&(resource_sync->cond_drain_requests), NULL))
		return -1;
	
	resource_sync->ongoing_count = 0;
	
	if(pthread_cond_init(&(resource_sync->cond_ongoing), NULL))
		return -1;
	
	return 0;
}

int destroy_resource_sync(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	
	if(pthread_mutex_destroy(&(resource_sync->mutex_resource)))
		return -1;
	
	resource_sync->drain_requests_count = 0;
	
	if(pthread_cond_destroy(&(resource_sync->cond_drain_requests)))
		return -1;
	
	resource_sync->ongoing_count = 0;
	
	if(pthread_cond_destroy(&(resource_sync->cond_ongoing)))
		return -1;

	return 0;
}

int wait_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync) {
    if(wait_ongoing_locking(resource_sync))
		return -1;
    
	if(pthread_mutex_unlock(&(resource_sync->mutex_resource)))
		return -1;

	return 0;
}

int signal_ongoing(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		return -1;
	}

		resource_sync->drain_requests_count--;

		// Acá se podría agregar un if para hacer el broadcast sólo si el semáforo efectivamente quedó en 0
		if(pthread_cond_broadcast(&(resource_sync->cond_drain_requests))) {
			return -1;
		}
	
	if(pthread_mutex_unlock(&(resource_sync->mutex_resource))) {
		return -1;
	}

	return 0;
}

int wait_ongoing_locking(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		return -1;
	}

		resource_sync->drain_requests_count++;

		while(1) {
			
			if(!(resource_sync->ongoing_count))
				break;
			
			if(pthread_cond_wait(&(resource_sync->cond_ongoing), &(resource_sync->mutex_resource))) {
				return -1;
			}
		}

	return 0;
}

int signal_ongoing_unlocking(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	
	if(pthread_mutex_unlock(&(resource_sync->mutex_resource))) {
		return -1;
	}

	if(signal_ongoing(resource_sync)) {
		return -1;
	}

	return 0;
}

int wait_draining_requests(t_Drain_Ongoing_Resource_Sync *resource_sync) {
    if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		return -1;
	}

		while(1) {
			if(!(resource_sync->drain_requests_count))
				break;

			if(pthread_cond_wait(&(resource_sync->cond_drain_requests), &(resource_sync->mutex_resource))) {
				return -1;
			}
		}

		resource_sync->ongoing_count++;
    
	if(pthread_mutex_unlock(&(resource_sync->mutex_resource))) {
		return -1;
	}

	return 0;
}

int signal_draining_requests(t_Drain_Ongoing_Resource_Sync *resource_sync) {
	if(pthread_mutex_lock(&(resource_sync->mutex_resource))) {
		return -1;
	}
	
		resource_sync->ongoing_count--;
		
		// Acá se podría agregar un if para hacer el signal sólo si el semáforo efectivamente quedó en 0
		if(pthread_cond_signal(&(resource_sync->cond_ongoing))) {// podría ser un broadcast en lugar de un wait si hay más de un comando de consola esperando
			return -1;
		}
	
	if(pthread_mutex_unlock(&(resource_sync->mutex_resource))) {
		return -1;
	}

	return 0;
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