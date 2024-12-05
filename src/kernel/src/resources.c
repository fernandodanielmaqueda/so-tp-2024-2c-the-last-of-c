/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "resources.h"

t_Resource *resource_create(int instances) {
	int retval = 0, status;

	t_Resource *resource = malloc(sizeof(t_Resource));
	if(resource == NULL) {
		log_error_r(&MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para un recurso", sizeof(t_Resource));
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) free, resource);

	resource->instances = instances;

	if((status = pthread_mutex_init(&(resource->mutex_resource), NULL))) {
		report_error_pthread_mutex_init(status);
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
			report_error_pthread_mutex_destroy(status);
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

int resource_destroy(t_Resource *resource) {
	int retval = 0, status;

	list_destroy(resource->list_blocked);

	if((status = pthread_mutex_destroy(&(resource->mutex_resource)))) {
		report_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	free(resource);

	return retval;
}

void resources_unassign(t_TCB *tcb) {
	int status;

	char *resource_name;
	t_Resource *resource;
	t_TCB *tcb_unblock = NULL;

	for(int table_index = 0; table_index < tcb->dictionary_assigned_resources->table_max_size; table_index++) {
		t_hash_element *element = tcb->dictionary_assigned_resources->elements[table_index];
		t_hash_element *next_element = NULL;

		while(element != NULL) {

			next_element = element->next;

			resource_name = element->key;
			resource = (t_Resource *) element->data;

			if((status = pthread_rwlock_rdlock(&(tcb->pcb->rwlock_resources)))) {
				report_error_pthread_rwlock_rdlock(status);
				exit_sigint();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(tcb->pcb->rwlock_resources));

				if((status = pthread_mutex_lock(&(resource->mutex_resource)))) {
					report_error_pthread_mutex_lock(status);
					exit_sigint();
				}
				pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(resource->mutex_resource));

					(resource->instances)++;

					if((resource->instances) <= 0) {

						if(get_state_blocked_mutex(&tcb_unblock, resource)) {
							exit_sigint();
						}

					}

				pthread_cleanup_pop(0); // mutex_resource
				if((status = pthread_mutex_unlock(&(resource->mutex_resource)))) {
					report_error_pthread_mutex_unlock(status);
					exit_sigint();
				}

			pthread_cleanup_pop(0); // rwlock_resources
			if((status = pthread_rwlock_unlock(&(tcb->pcb->rwlock_resources)))) {
				report_error_pthread_rwlock_unlock(status);
				exit_sigint();
			}

			if(tcb_unblock != NULL) {
				dictionary_put(tcb_unblock->dictionary_assigned_resources, resource_name, resource);

				if(insert_state_ready(tcb_unblock)) {
					exit_sigint();
				}
			}

			//free(element->key);
			//free(element);

			element = next_element;
		}

		//tcb->dictionary_assigned_resources->elements[table_index] = NULL;
	}

	//tcb->dictionary_assigned_resources->table_current_size = 0;
	//tcb->dictionary_assigned_resources->elements_amount = 0;

	//free(tcb->dictionary_assigned_resources->elements);
	//free(tcb->dictionary_assigned_resources);

	return;
}