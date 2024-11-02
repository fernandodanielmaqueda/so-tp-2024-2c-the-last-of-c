/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "resources.h"

t_Resource *resource_create(void) {
	int retval = 0, status;

	t_Resource *resource = malloc(sizeof(t_Resource));
	if(resource == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para un recurso", sizeof(t_Resource));
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) free, resource);

	if((status = pthread_mutex_init(&(resource->mutex_resource), NULL))) {
		log_error_pthread_mutex_init(status);
		retval = -1;
		goto cleanup_resource;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(resource->mutex_resource));

	resource->list_blocked = list_create();
	if(resource->list_blocked == NULL) {
		retval = -1;
		goto cleanup_mutex;
	}

	cleanup_mutex:
	pthread_cleanup_pop(0); // resource->list_blocked.mutex
	if(retval) {
		if((status = pthread_mutex_destroy(&(resource->mutex_resource)))) {
			log_error_pthread_mutex_destroy(status);
		}
	}
	cleanup_resource:
	pthread_cleanup_pop(retval); // resource
	ret:
	if(retval)
		return NULL;
	else
		return resource;
}

void resource_destroy(t_Resource *resource) {
	/*
	int status;

	list_destroy_and_destroy_elements(resource->shared_list_blocked.list, free);
	if((status = pthread_mutex_destroy(&(resource->shared_list_blocked.mutex)))) {
		log_error_pthread_mutex_destroy(status);
		// TODO
	}

	free(resource);
	*/
}