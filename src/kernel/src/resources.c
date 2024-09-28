/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "resources.h"

t_Resource *resource_create(void) {
	int status;

	t_Resource *resource = malloc(sizeof(t_Resource));
	if(resource == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para un recurso", sizeof(t_Resource));
		return NULL;
	}

	resource->shared_list_blocked.list = list_create();
	if((status = pthread_mutex_init(&(resource->shared_list_blocked.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		// TODO
	}

	return resource;
}

void resource_destroy(t_Resource *resource) {
	int status;

	list_destroy_and_destroy_elements(resource->shared_list_blocked.list, free);
	if((status = pthread_mutex_destroy(&(resource->shared_list_blocked.mutex)))) {
		log_error_pthread_mutex_destroy(status);
		// TODO
	}

	free(resource);
}