/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "resources.h"

int RESOURCE_QUANTITY;
t_Resource *RESOURCES;

void resources_read_module_config(t_config *module_config) {
	char **resource_names = config_get_array_value(module_config, "RECURSOS");
	char **resource_instances = config_get_array_value(module_config, "INSTANCIAS_RECURSOS");

	for(RESOURCE_QUANTITY = 0; (resource_names[RESOURCE_QUANTITY] != NULL) && (resource_instances[RESOURCE_QUANTITY] != NULL); RESOURCE_QUANTITY++);
	
	if((resource_names[RESOURCE_QUANTITY] != NULL) || (resource_instances[RESOURCE_QUANTITY] != NULL)) {
		log_error(MODULE_LOGGER, "La cantidad de recursos y de instancias de recursos no coinciden");
		exit(EXIT_FAILURE);
	}

	if(RESOURCE_QUANTITY > 0) {
		RESOURCES = malloc(sizeof(t_Resource) * RESOURCE_QUANTITY);
		if(RESOURCES == NULL) {
			log_error(MODULE_LOGGER, "No se pudo reservar memoria para los recursos");
			exit(EXIT_FAILURE);
		}
	}

	char *end;

	for(register int i = 0; i < RESOURCE_QUANTITY; i++) {
		RESOURCES[i].instances = strtol(resource_instances[i], &end, 10);
		if(!*(resource_instances[i]) || *end) {
			log_error(MODULE_LOGGER, "La cantidad de instancias del recurso %s no es un número válido: %s", resource_names[i], resource_instances[i]);
			exit(EXIT_FAILURE);
		}

		RESOURCES[i].name = resource_names[i];
		RESOURCES[i].shared_list_blocked.list = list_create();
		pthread_mutex_init(&(RESOURCES[i].shared_list_blocked.mutex), NULL);
	}
}

t_Resource *resource_find(char *name) {
    for(register int i = 0; i < RESOURCE_QUANTITY; i++)
        if(strcmp((RESOURCES[i]).name, name) == 0)
            return (&(RESOURCES[i]));

    return NULL;
}

void resource_log(t_Resource *resource) {
	log_trace(MODULE_LOGGER, "Recurso: %s - Instancias %ld", resource->name, resource->instances);
}

void resources_free(void) {
	free(RESOURCES);
	// TODO: Liberar lista de bloqueados
}