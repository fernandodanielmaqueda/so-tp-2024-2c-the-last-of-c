/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "kernel.h"

char *MODULE_NAME = "kernel";

t_log *MODULE_LOGGER = NULL;
char *MODULE_LOG_PATHNAME = "kernel.log";

t_config *MODULE_CONFIG = NULL;
char *MODULE_CONFIG_PATHNAME = "kernel.config";

t_ID_Manager PID_MANAGER;

const char *STATE_NAMES[] = {
	[NEW_STATE] = "NEW",
	[READY_STATE] = "READY",
	[EXEC_STATE] = "EXEC",
	[BLOCKED_STATE] = "BLOCKED",
	[EXIT_STATE] = "EXIT"
};

char *SCHEDULING_ALGORITHMS[] = {
	[FIFO_SCHEDULING_ALGORITHM] = "FIFO",
	[PRIORITIES_SCHEDULING_ALGORITHM] = "PRIORIDADES",
	[MLQ_SCHEDULING_ALGORITHM] = "CMN"
};

e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

int module(int argc, char *argv[]) {

	if(argc < 3) {
		fprintf(stderr, "Uso: %s <ARCHIVO_PSEUDOCODIGO> <TAMANIO_PROCESO> [ARGUMENTOS]\n", argv[0]);
		goto error;
	}

	size_t process_size;
	if(str_to_size(argv[2], &process_size)) {
		fprintf(stderr, "%s: No es un TAMANIO_PROCESO valido\n", argv[2]);
		goto error;
	}

	if(initialize_configs(MODULE_CONFIG_PATHNAME)) {
		goto error;
	}

	if(initialize_loggers()) {
		goto error_configs;
	}

	if(initialize_global_variables()) {
		goto error_loggers;
	}

	if(initialize_sockets()) {
		goto error_global_variables;
	}

	if(initialize_long_term_scheduler()) {
		goto error_sockets;
	}

	if(create_pthread(&THREAD_CPU_INTERRUPTER, (void *(*)(void *)) cpu_interrupter, NULL)) {
		goto error_long_term_scheduler;
	}

	if(new_process(process_size, argv[1], 0)) {
        log_error(MODULE_LOGGER, "No se pudo crear el proceso");
        goto error_cpu_interrupter;
    }

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	short_term_scheduler(NULL);

	return EXIT_SUCCESS;

	error_cpu_interrupter:
		cancel_pthread(&THREAD_CPU_INTERRUPTER);
	error_long_term_scheduler:
		finish_long_term_scheduler();
	error_sockets:
		finish_sockets();
	error_global_variables:
		finish_global_variables();
	error_loggers:
		finish_loggers();
	error_configs:
		finish_configs();
	error:
		return EXIT_FAILURE;
}

int read_module_config(t_config *module_config) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "IP_CPU", "PUERTO_CPU_DISPATCH", "PUERTO_CPU_INTERRUPT", "ALGORITMO_PLANIFICACION", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

	CONNECTION_CPU_DISPATCH = (t_Connection) {.client_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .server_type = CPU_DISPATCH_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_DISPATCH")};
	CONNECTION_CPU_INTERRUPT = (t_Connection) {.client_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .server_type = CPU_INTERRUPT_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_INTERRUPT")};

	char *string = config_get_string_value(module_config, "ALGORITMO_PLANIFICACION");
	if(find_scheduling_algorithm(string, &SCHEDULING_ALGORITHM)) {
		fprintf(stderr, "%s: valor de la clave ALGORITMO_PLANIFICACION invalido: %s\n", MODULE_CONFIG_PATHNAME, string);
		return -1;
	}

	if(SCHEDULING_ALGORITHM == MLQ_SCHEDULING_ALGORITHM) {
		if(!config_has_property(module_config, "QUANTUM")) {
			fprintf(stderr, "%s: El archivo de configuración no contiene la clave %s\n", module_config->path, "QUANTUM");
			fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
			return -1;
		}

		QUANTUM = config_get_int_value(module_config, "QUANTUM");
	}

	LOG_LEVEL = log_level_from_string(config_get_string_value(module_config, "LOG_LEVEL"));

	return 0;
}

int find_scheduling_algorithm(char *name, e_Scheduling_Algorithm *destination) {

    if(name == NULL || destination == NULL)
        return -1;
    
    size_t scheduling_algorithms_number = sizeof(SCHEDULING_ALGORITHMS) / sizeof(SCHEDULING_ALGORITHMS[0]);
    for(register e_Scheduling_Algorithm scheduling_algorithm = 0; scheduling_algorithm < scheduling_algorithms_number; scheduling_algorithm++)
        if(strcmp(SCHEDULING_ALGORITHMS[scheduling_algorithm], name) == 0) {
            *destination = scheduling_algorithm;
            return 0;
        }

    return -1;
}

int initialize_global_variables(void) {
	int status;

	if((status = pthread_mutex_init(&(SHARED_LIST_NEW.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		goto error;
	}

	SHARED_LIST_NEW.list = list_create();
	if(SHARED_LIST_NEW.list == NULL) {
		log_error(MODULE_LOGGER, "list_create: No se pudo crear la lista de procesos en NEW");
		goto error_mutex_new;
	}

	if(init_resource_sync(&READY_SYNC)) {
		goto error_list_new;
	}

	request_ready_list(0);

	if((status = pthread_mutex_init(&(SHARED_LIST_EXEC.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		goto error_ready_sync;
	}

	SHARED_LIST_EXEC.list = list_create();
	if(SHARED_LIST_EXEC.list == NULL) {
		log_error(MODULE_LOGGER, "list_create: No se pudo crear la lista de procesos en EXEC");
		goto error_mutex_exec;
	}

	if((status = pthread_mutex_init(&(SHARED_LIST_EXIT.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		goto error_list_exec;
	}

	SHARED_LIST_EXIT.list = list_create();
	if(SHARED_LIST_EXIT.list == NULL) {
		log_error(MODULE_LOGGER, "list_create: No se pudo crear la lista de procesos en EXIT");
		goto error_mutex_exit;
	}

	if(sem_init(&SEM_LONG_TERM_SCHEDULER_NEW, 0, 0)) {
		log_error_sem_init();
		goto error_list_exit;
	}

	if(sem_init(&SEM_LONG_TERM_SCHEDULER_EXIT, 0, 0)) {
		log_error_sem_init();
		goto error_sem_long_term_scheduler_new;
	}

	if(sem_init(&SEM_SHORT_TERM_SCHEDULER, 0, 0)) {
		log_error_sem_init();
		goto error_sem_long_term_scheduler_exit;
	}

	if((status = pthread_mutex_init(&MUTEX_QUANTUM_INTERRUPT, NULL))) {
		log_error_pthread_mutex_init(status);
		goto error_sem_short_term_scheduler;
	}

	return 0;

	error_sem_short_term_scheduler:
		if(sem_destroy(&SEM_SHORT_TERM_SCHEDULER)) {
			log_error_sem_destroy();
		}
	error_sem_long_term_scheduler_exit:
		if(sem_destroy(&SEM_LONG_TERM_SCHEDULER_EXIT)) {
			log_error_sem_destroy();
		}
	error_sem_long_term_scheduler_new:
		if(sem_destroy(&SEM_LONG_TERM_SCHEDULER_NEW)) {
			log_error_sem_destroy();
		}
	error_list_exit:
		list_destroy_and_destroy_elements(SHARED_LIST_EXIT.list, (void (*)(void *)) tcb_destroy);
	error_mutex_exit:
		if((status = pthread_mutex_destroy(&(SHARED_LIST_EXIT.mutex)))) {
			log_error_pthread_mutex_destroy(status);
		}
	error_list_exec:
		list_destroy_and_destroy_elements(SHARED_LIST_EXEC.list, (void (*)(void *)) tcb_destroy);
	error_mutex_exec:
		if((status = pthread_mutex_destroy(&(SHARED_LIST_EXEC.mutex)))) {

		}
	error_ready_sync:
		destroy_resource_sync(&READY_SYNC);
	error_list_new:
		list_destroy_and_destroy_elements(SHARED_LIST_NEW.list, (void (*)(void *)) pcb_destroy);
	error_mutex_new:
		if((status = pthread_mutex_destroy(&(SHARED_LIST_NEW.mutex)))) {
			log_error_pthread_mutex_destroy(status);
		}
	error:
		return -1;
}

int finish_global_variables(void) {
	int retval = 0, status;

	if((status = pthread_mutex_destroy(&MUTEX_QUANTUM_INTERRUPT))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	if(sem_destroy(&SEM_SHORT_TERM_SCHEDULER)) {
		log_error_sem_destroy();
		retval = -1;
	}

	if(sem_destroy(&SEM_LONG_TERM_SCHEDULER_EXIT)) {
		log_error_sem_destroy();
		retval = -1;
	}

	if(sem_destroy(&SEM_LONG_TERM_SCHEDULER_NEW)) {
		log_error_sem_destroy();
		retval = -1;
	}

	if((status = pthread_mutex_destroy(&(SHARED_LIST_EXIT.mutex)))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	list_destroy_and_destroy_elements(SHARED_LIST_EXIT.list, (void (*)(void *)) tcb_destroy);

	if((status = pthread_mutex_destroy(&(SHARED_LIST_EXEC.mutex)))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	list_destroy_and_destroy_elements(SHARED_LIST_EXEC.list, (void (*)(void *)) tcb_destroy);

	destroy_resource_sync(&READY_SYNC);

	if((status = pthread_mutex_destroy(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	list_destroy_and_destroy_elements(SHARED_LIST_NEW.list, (void (*)(void *)) pcb_destroy);

	return retval;
}

t_PCB *pcb_create(size_t size) {

	t_PCB *pcb = malloc(sizeof(t_PCB));
	if(pcb == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el PCB", sizeof(t_PCB));
		return NULL;
	}

	pcb->size = size;

	id_manager_init(&(pcb->thread_manager), THREAD_ID_MANAGER_TYPE);

	pcb->dictionary_mutexes = dictionary_create();

	pid_assign(&PID_MANAGER, pcb, &(pcb->PID));

	return pcb;
}

void pcb_destroy(t_PCB *pcb) {
	pid_release(&PID_MANAGER, pcb->PID);

	dictionary_destroy_and_destroy_elements(pcb->dictionary_mutexes, (void (*)(void *)) resource_destroy);

	free(pcb);
}

t_TCB *tcb_create(t_PCB *pcb, char *pseudocode_filename, t_Priority priority) {
	int status;

	t_TCB *tcb = malloc(sizeof(t_TCB));
	if(tcb == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el TCB", sizeof(t_TCB));
		return NULL;
	}

	tcb->pcb = pcb;

	tcb->pseudocode_filename = pseudocode_filename;
	tcb->priority = priority;

	tcb->current_state = NEW_STATE;
	tcb->shared_list_state = NULL;

	tcb->quantum = QUANTUM;

	payload_init(&(tcb->syscall_instruction));

	tcb->shared_list_blocked_thread_join.list = list_create();
	if((status = pthread_mutex_init(&(tcb->shared_list_blocked_thread_join.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		// TODO
	}

	tcb->dictionary_assigned_mutexes = dictionary_create();

	tid_assign(&(pcb->thread_manager), tcb, &(tcb->TID));

	return tcb;
}

void tcb_destroy(t_TCB *tcb) {
	int status;

	tid_release(&(tcb->pcb->thread_manager), tcb->TID);
	
	free(tcb->pseudocode_filename);

	payload_destroy(&(tcb->syscall_instruction));

	list_destroy_and_destroy_elements(tcb->shared_list_blocked_thread_join.list, (void (*)(void *)) pcb_destroy);

	if((status = pthread_mutex_destroy(&(tcb->shared_list_blocked_thread_join.mutex)))) {
		log_error_pthread_mutex_destroy(status);
		// TODO
	}

	dictionary_destroy(tcb->dictionary_assigned_mutexes);

	free(tcb);
}

int id_manager_init(t_ID_Manager *id_manager, e_ID_Manager_Type id_data_type) {
	/*
	if(id_manager == NULL) {
		log_error(MODULE_LOGGER, "id_manager_init: %s", strerror(EINVAL));
		errno = EINVAL;
		return -1;
	}

	int status;

	id_manager->id_data_type = id_data_type;

	if(status = pthread_mutex_init(&(id_manager->mutex_cb_array), NULL)) {
		free(id_manager->id_counter);
		errno = status;
		return -1;
	}

	id_manager->id_counter = malloc(get_id_size(id_data_type));
	if(id_manager->id_counter == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el contador de IDs", get_id_size(id_data_type));
		errno = ENOMEM;
		goto error;
	}
	size_to_id(id_data_type, id_manager->id_counter, &(size_t){0});

	id_manager->cb_array = NULL;

	id_manager->list_released_ids = list_create();

	if(status = _pthread_mutex_init(&(id_manager->mutex_list_released_ids), NULL)) {
		free(id_manager->id_counter);
		free(id_manager->cb_array);
		if(status = pthread_mutex_destroy(&(id_manager->mutex_cb_array))) {
			log_error_pthread_mutex_destroy(status);
			// TODO
		}
		errno = status;
		return -1;
	}

	if(status = _pthread_cond_init(&(id_manager->cond_list_released_ids), NULL)) {
		free(id_manager->id_counter);
		free(id_manager->cb_array);
		if(status = _pthread_mutex_destroy(&(id_manager->mutex_cb_array));
		list_destroy_and_destroy_elements(id_manager->list_released_ids, free);
		if(status = _pthread_mutex_destroy(&(id_manager->mutex_list_released_ids));
		errno = status;
		return -1;
	}
	*/

	return 0;

	/*
	error_mutex_cb_array:
		if(status = _pthread_mutex_destroy(&(id_manager->mutex_cb_array))) {
			log_error_pthread_mutex_destroy(status);
		}
	error:
		return -1;
	*/
}

void id_manager_destroy(t_ID_Manager *id_manager) {
	/*
	free(id_manager->id_counter);
	free(id_manager->cb_array);
	if(status = _pthread_mutex_destroy(&(id_manager->mutex_cb_array));
	list_destroy_and_destroy_elements(id_manager->list_released_ids, free);
	if(status = _pthread_mutex_destroy(&(id_manager->mutex_list_released_ids));
	if(status = _pthread_cond_destroy(&(id_manager->cond_list_released_ids));
	*/
}

int _id_assign(t_ID_Manager *id_manager, void *data, void *result) {
	/*
	if(id_manager == NULL || result == NULL) {
		errno = EINVAL;
		return -1;
	}

	int status;
	size_t id_generic;

	status = if(status = _pthread_mutex_lock(&(id_manager->mutex_list_released_ids));
	if(status) {
		errno = status;
		return -1;
	}
		// Convierto el tipo de dato del ID a uno genérico (size_t) para poder operar con él en la función
		if(id_to_size(id_manager->id_data_type, id_manager->id_counter, &id_generic)) {
			if(status = _pthread_mutex_unlock(&(id_manager->mutex_list_released_ids));
			return -1;
		}

		// Si no se alcanzó el máximo de ID (sin fijarse si hay o no ID liberados), se asigna un nuevo ID
		if(id_generic <= get_id_max(id_manager->id_data_type)) {
			status = if(status = _pthread_mutex_unlock(&(id_manager->mutex_list_released_ids));
			if(status) {
				errno = status;
				return -1;
			}

			status = if(status = _pthread_mutex_lock(&(id_manager->mutex_cb_array));
			if(status) {
				errno = status;
				return -1;
			}

				void **new_cb_array = realloc(id_manager->cb_array, sizeof(void *) * (id_generic + 1));
				if(new_cb_array == NULL) {
					log_error(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(void *) * id_generic, sizeof(void *) * (id_generic + 1));
					if(status = _pthread_mutex_unlock(&(id_manager->mutex_cb_array));
					errno = ENOMEM;
					return -1;
				}
				id_manager->cb_array = new_cb_array;

				(id_manager->cb_array)[id_generic] = data;
				
			status = if(status = _pthread_mutex_unlock(&(id_manager->mutex_cb_array));
			if(status) {
				errno = status;
				return -1;
			}

			// Se convierte el ID asignado a su tipo de dato correspondiente como resultado
			if(size_to_id(id_manager->id_data_type, &id_generic, result)) {
				return -1;
			}

			id_generic++;
			
			// Se actualiza el contador de IDs
			if(size_to_id(id_manager->id_data_type, &id_generic, id_manager->id_counter)) {
				return -1;
			}

			return 0;
		}

		// En caso contrario, se espera hasta que haya algún ID liberado para reutilizarlo
		while(id_manager->list_released_ids->head == NULL) {
			status = if(status = _pthread_cond_wait(&(id_manager->cond_list_released_ids), &(id_manager->mutex_list_released_ids));
			if(status) {
				errno = status;
				return -1;
			}
		}

		t_link_element *element = id_manager->list_released_ids->head;

		id_manager->list_released_ids->head = element->next;
		id_manager->list_released_ids->elements_count--;
	
	status = if(status = _pthread_mutex_unlock(&(id_manager->mutex_list_released_ids));
	if(status) {
		errno = status;
		return -1;
	}

	// Se determina el ID liberado como el resultado
	memcpy(result, element->data, get_id_size(id_manager->id_data_type));

	// Convierto el tipo de dato del ID a uno genérico (size_t) para poder operar con él en la función
	status = id_to_size(id_manager->id_data_type, element->data, &id_generic);
	if(status) {
		return -1;
	}

	free(element->data);
	free(element);

	status = if(status = _pthread_mutex_lock(&(id_manager->mutex_cb_array));
	if(status) {
		errno = status;
		return -1;
	}

		(id_manager->cb_array)[id_generic] = data;

	status = if(status = _pthread_mutex_unlock(&(id_manager->mutex_cb_array));
	if(status) {
		errno = status;
		return -1;
	}
	*/

	return 0;
}

int pid_assign(t_ID_Manager *id_manager, t_PCB *pcb, t_PID *result) {
	return _id_assign(id_manager, (void *) pcb, (void *) result);
}

int tid_assign(t_ID_Manager *id_manager, t_TCB *tcb, t_TID *result) {
	return _id_assign(id_manager, (void *) tcb, (void *) result);
}

int _id_release(t_ID_Manager *id_manager, size_t id, void *data) {
	/*
	if(status = _pthread_mutex_lock(&(id_manager->mutex_cb_array));
		(id_manager->cb_array)[id] = NULL;
	if(status = _pthread_mutex_unlock(&(id_manager->mutex_cb_array));

	t_link_element *element = malloc(sizeof(t_link_element));
	if(element == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para un nuevo elemento en la lista de IDs libres", sizeof(t_link_element));
		exit(EXIT_FAILURE);
	}

	element->data = data;

	if(status = _pthread_mutex_lock(&(id_manager->mutex_list_released_ids));
		element->next = id_manager->list_released_ids->head;
		id_manager->list_released_ids->head = element;
	if(status = _pthread_mutex_unlock(&(id_manager->mutex_list_released_ids));

	if(status = _pthread_cond_signal(&(id_manager->cond_list_released_ids));
	*/

	return 0;
}

int pid_release(t_ID_Manager *id_manager, t_PID pid) {
	t_PID *pid_copy = malloc(sizeof(t_PID));
	if(pid_copy == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el PID", sizeof(t_PID));
		exit(EXIT_FAILURE);
	}
	*pid_copy = pid;

	_id_release(id_manager, (size_t) pid, (void *) pid_copy);

	return 0;
}

int tid_release(t_ID_Manager *id_manager, t_TID tid) {
	t_TID *tid_copy = malloc(sizeof(t_TID));
	if(tid_copy == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el TID", sizeof(t_TID));
		exit(EXIT_FAILURE);
	}
	*tid_copy = tid;

	_id_release(id_manager, (size_t) tid, (void *) tid_copy);

	return 0;
}

size_t get_id_max(e_ID_Manager_Type id_manager_type) {
	switch(id_manager_type) {
		case PROCESS_ID_MANAGER_TYPE:
			return PID_MAX;
		case THREAD_ID_MANAGER_TYPE:
			return TID_MAX;
	}

	return 0;
}

size_t get_id_size(e_ID_Manager_Type id_manager_type) {
	switch(id_manager_type) {
		case PROCESS_ID_MANAGER_TYPE:
			return sizeof(t_PID);
		case THREAD_ID_MANAGER_TYPE:
			return sizeof(t_TID);
	}

	return 0;
}

int id_to_size(e_ID_Manager_Type id_data_type, void *source, size_t *destination) {
	if(destination == NULL || source == NULL) {
		log_error(MODULE_LOGGER, "id_to_size: %s", strerror(EINVAL));
		errno = EINVAL;
		return -1;
	}

	switch(id_data_type) {
		case PROCESS_ID_MANAGER_TYPE:
			*destination = *((t_PID *) source);
		case THREAD_ID_MANAGER_TYPE:
			*destination = *((t_TID *) source);
	}

	return 0;
}

int size_to_id(e_ID_Manager_Type id_data_type, size_t *source, void *destination) {
	if(destination == NULL || source == NULL) {
		log_error(MODULE_LOGGER, "size_to_id: %s", strerror(EINVAL));
		errno = EINVAL;
		return -1;
	}

	switch(id_data_type) {
		case PROCESS_ID_MANAGER_TYPE:
			*((t_PID *) destination) = (t_PID) (*source);
		case THREAD_ID_MANAGER_TYPE:
			*((t_TID *) destination) = (t_TID) (*source);
	}

	return 0;
}

bool pcb_matches_pid(t_PCB *pcb, t_PID *pid) {
	return pcb->PID == *pid;
}

bool tcb_matches_tid(t_TCB *tcb, t_TID *tid) {
	return tcb->TID == *tid;
}

int new_process(size_t size, char *pseudocode_filename, t_Priority priority) {
	int status;

	t_PCB *pcb = pcb_create(size);
	if(pcb == NULL) {
		log_error(MODULE_LOGGER, "pcb_create: No se pudo crear el PCB");
		goto error;
	}

	t_TCB *tcb = tcb_create(pcb, pseudocode_filename, priority);
	if(tcb == NULL) {
		log_error(MODULE_LOGGER, "tcb_create: No se pudo crear el TCB");
		goto error_pcb;
	}

	if((status = pthread_mutex_lock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_lock(status);
		goto error_tcb;
	}

		list_add(SHARED_LIST_NEW.list, pcb);

	if((status = pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		goto error_tcb;
	}

	log_info(MINIMAL_LOGGER, "## (<%u>:%u) Se crea el proceso - Estado: NEW", pcb->PID, tcb->TID);

	if(sem_post(&SEM_LONG_TERM_SCHEDULER_NEW)) {
		log_error_sem_post();
		goto error;
	}

	return 0;

	error_tcb:
		tcb_destroy(tcb);
	error_pcb:
		pcb_destroy(pcb);
	error:
		return -1;
}

int request_ready_list(t_Priority priority) { PRIORITY_MAX
	int status;

	/*
	ARRAY_LIST_READY[0]
	ARRAY_LIST_READY[1]
	ARRAY_LIST_READY[2] PRIORITY_COUNT = 2 + 1

	4 priority PRIORITY_MAX
	*/

	if(priority < PRIORITY_COUNT) {
		return 0;
	}

	// TODO: READY_SYNC

		t_Shared_List *new_array_list_ready = realloc(ARRAY_LIST_READY, sizeof(t_Shared_List) * (priority + 1));
		if(new_array_list_ready == NULL) {
			log_error(MODULE_LOGGER, "realloc: No se pudieron reservar %zu bytes para la nueva lista de procesos en READY", sizeof(t_Shared_List));
			return -1;
		}
		ARRAY_LIST_READY = new_array_list_ready;
		
		for(t_Priority i = PRIORITY_COUNT; i < priority; i++) {
			if((status = pthread_mutex_init(&(ARRAY_LIST_READY[i].mutex), NULL))) {
				log_error_pthread_mutex_init(status);
				//goto error;
			}

			ARRAY_LIST_READY[i].list = list_create();
			if(ARRAY_LIST_READY[i].list == NULL) {
				log_error(MODULE_LOGGER, "list_create: No se pudo crear la lista de procesos en NEW");
				//goto error_mutex_new;
			}
		}

		// Tomo el UINT_MAX aparte
		if(PRIORITY_COUNT == priority) {
			if((status = pthread_mutex_init(&(ARRAY_LIST_READY[i].mutex), NULL))) {
				log_error_pthread_mutex_init(status);
				//goto error;
			}

			ARRAY_LIST_READY[i].list = list_create();
			if(ARRAY_LIST_READY[i].list == NULL) {
				log_error(MODULE_LOGGER, "list_create: No se pudo crear la lista de procesos en NEW");
				//goto error_mutex_new;
			}
		}

		PRIORITY_COUNT = priority + 1;
	
	// TODO: FIN SECCIÓN CRÍTICA

	return 0;

	error:
		return -1;
}

void log_state_list(t_log *logger, const char *state_name, t_list *pcb_list) {
	char *pid_string = string_new();
	pcb_list_to_pid_string(pcb_list, &pid_string);
	log_info(logger, "%s: [%s]", state_name, pid_string);
	free(pid_string);
}

void pcb_list_to_pid_string(t_list *pcb_list, char **destination) {
	if(pcb_list == NULL || destination == NULL || *destination == NULL)
		return;

	t_link_element *element = pcb_list->head;

	if(**destination && element != NULL)
		string_append(destination, ", ");

	char *pid_as_string;
	while(element != NULL) {
        pid_as_string = string_from_format("%" PRIu32, ((t_PCB *) element->data)->PID);
        string_append(destination, pid_as_string);
        free(pid_as_string);
		element = element->next;
        if(element != NULL)
            string_append(destination, ", ");
    }
}