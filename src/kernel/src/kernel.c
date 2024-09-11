/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "kernel.h"

char *MODULE_NAME = "kernel";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "kernel.log";

t_config *MODULE_CONFIG;
char *MODULE_CONFIG_PATHNAME = "kernel.config";

t_ID_Manager PID_MANAGER;

const char *STATE_NAMES[] = {
	[NEW_STATE] = "NEW",
	[READY_STATE] = "READY",
	[EXEC_STATE] = "EXEC",
	[BLOCKED_STATE] = "BLOCKED",
	[EXIT_STATE] = "EXIT"
};

int module(int argc, char *argv[]) {

	if(argc < 3) {
		fprintf(stderr, "Uso: %s <ARCHIVO_PSEUDOCODIGO> <TAMANIO_PROCESO> [ARGUMENTOS]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	initialize_configs(MODULE_CONFIG_PATHNAME);
	initialize_loggers();

	initialize_global_variables();

	initialize_sockets();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	initialize_scheduling();

	finish_scheduling();
	//finish_threads();
	finish_sockets();
	//finish_configs();
	finish_loggers();
	finish_global_variables();

    return EXIT_SUCCESS;
}

void initialize_global_variables(void) {

	SHARED_LIST_NEW.list = list_create();
	pthread_mutex_init(&(SHARED_LIST_NEW.mutex), NULL);

	init_resource_sync(&READY_SYNC);

	SHARED_LIST_EXEC.list = list_create();
	pthread_mutex_init(&(SHARED_LIST_EXEC.mutex), NULL);

	SHARED_LIST_EXIT.list = list_create();
	pthread_mutex_init(&(SHARED_LIST_EXIT.mutex), NULL);

	sem_init(&SEM_LONG_TERM_SCHEDULER_NEW, 0, 0);
	sem_init(&SEM_LONG_TERM_SCHEDULER_EXIT, 0, 0);
	sem_init(&SEM_SHORT_TERM_SCHEDULER, 0, 0);

	pthread_mutex_init(&MUTEX_QUANTUM_INTERRUPT, NULL);
}

void finish_global_variables(void) {
	list_destroy_and_destroy_elements(SHARED_LIST_NEW.list, (void (*)(void *)) pcb_destroy);
	pthread_mutex_destroy(&(SHARED_LIST_NEW.mutex));
	
	destroy_resource_sync(&READY_SYNC);

	list_destroy_and_destroy_elements(SHARED_LIST_EXEC.list, (void (*)(void *)) tcb_destroy);
	pthread_mutex_destroy(&(SHARED_LIST_EXEC.mutex));

	list_destroy_and_destroy_elements(SHARED_LIST_EXIT.list, (void (*)(void *)) tcb_destroy);
	pthread_mutex_destroy(&(SHARED_LIST_EXIT.mutex));

	sem_destroy(&SEM_LONG_TERM_SCHEDULER_NEW);
	sem_destroy(&SEM_LONG_TERM_SCHEDULER_EXIT);
	sem_destroy(&SEM_SHORT_TERM_SCHEDULER);

	pthread_mutex_destroy(&MUTEX_QUANTUM_INTERRUPT);
}

void read_module_config(t_config *module_config) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "IP_CPU", "PUERTO_CPU_DISPATCH", "PUERTO_CPU_INTERRUPT", "ALGORITMO_PLANIFICACION", "QUANTUM", "LOG_LEVEL", NULL)) {
        //fprintf(stderr, "%s: El archivo de configuración no tiene la propiedad/key/clave %s", MODULE_CONFIG_PATHNAME, "LOG_LEVEL");
        exit(EXIT_FAILURE);
    }
	
	CONNECTION_CPU_DISPATCH = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = CPU_DISPATCH_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_DISPATCH")};
	CONNECTION_CPU_INTERRUPT = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = CPU_INTERRUPT_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_INTERRUPT")};
	
	if(find_scheduling_algorithm(config_get_string_value(module_config, "ALGORITMO_PLANIFICACION"), &SCHEDULING_ALGORITHM)) {
		fprintf(stderr, "ALGORITMO_PLANIFICACION invalido");
		exit(EXIT_FAILURE);
	}

	QUANTUM = config_get_int_value(module_config, "QUANTUM");
	LOG_LEVEL = log_level_from_string(config_get_string_value(module_config, "LOG_LEVEL"));
}

t_PCB *pcb_create(void) {

	t_PCB *pcb = malloc(sizeof(t_PCB));
	if(pcb == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el PCB", sizeof(t_PCB));
		return NULL;
	}

	pcb->PID = pid_assign(&PID_MANAGER, pcb);
	id_manager_init(&(pcb->thread_manager), THREAD_ID_MANAGER_TYPE);
	pcb->list_mutexes = list_create();

	//pcb->exec_context.quantum = QUANTUM;

	//payload_init(&(pcb->syscall_instruction));

	return pcb;
}

void pcb_destroy(t_PCB *pcb) {
	//list_destroy(pcb->assigned_resources);

	//payload_destroy(&(pcb->syscall_instruction));

	free(pcb);
}

t_TCB *tcb_create(t_PCB *pcb) {	

	t_TCB *tcb = malloc(sizeof(t_TCB));
	if(tcb == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el PCB", sizeof(t_TCB));
		return NULL;
	}

	tcb->TID = tid_assign(&(pcb->thread_manager), tcb);
	//pcb->list_tids = list_create();
	pcb->list_mutexes = list_create();

	//pcb->current_state = NEW_STATE;
	//pcb->shared_list_state = NULL;

	//payload_init(&(pcb->syscall_instruction));

	return tcb;
}

void tcb_destroy(t_TCB *tcb) {

	free(tcb);
}

int id_manager_init(t_ID_Manager *id_manager, e_ID_Manager_Type id_data_type) {
	if(id_manager == NULL) {
		log_error(MODULE_LOGGER, "id_manager_init: el ID_Manager pasado por parametro es NULL");
		errno = EINVAL;
		return -1;
	}

	int exit_value = 0;

	id_manager->id_data_type = id_data_type;

	id_manager->id_counter = malloc(get_id_size(id_data_type));
	if(id_manager->id_counter == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el contador de IDs", get_id_size(id_data_type));
		errno = ENOMEM;
		return -1;
	}
	size_to_id(id_data_type, id_manager->id_counter, &(size_t){0});

	id_manager->cb_array = NULL;

	exit_value = pthread_mutex_init(&(id_manager->mutex_cb_array), NULL);
	if(exit_value) {
		free(id_manager->id_counter);
		errno = exit_value;
		return -1;
	}

	id_manager->list_released_ids = list_create();

	exit_value = pthread_mutex_init(&(id_manager->mutex_list_released_ids), NULL);
	if(exit_value) {
		free(id_manager->id_counter);
		free(id_manager->cb_array);
		pthread_mutex_destroy(&(id_manager->mutex_cb_array));
		errno = exit_value;
		return -1;
	}

	exit_value = pthread_cond_init(&(id_manager->cond_list_released_ids), NULL);
	if(exit_value) {
		free(id_manager->id_counter);
		free(id_manager->cb_array);
		pthread_mutex_destroy(&(id_manager->mutex_cb_array));
		list_destroy_and_destroy_elements(id_manager->list_released_ids, free);
		pthread_mutex_destroy(&(id_manager->mutex_list_released_ids));
		errno = exit_value;
		return -1;
	}

	return 0;
}

void id_manager_destroy(t_ID_Manager *id_manager) {
	free(id_manager->id_counter);
	free(id_manager->cb_array);
	pthread_mutex_destroy(&(id_manager->mutex_cb_array));
	list_destroy_and_destroy_elements(id_manager->list_released_ids, free);
	pthread_mutex_destroy(&(id_manager->mutex_list_released_ids));
	pthread_cond_destroy(&(id_manager->cond_list_released_ids));
}

size_t _id_assign(t_ID_Manager *id_manager, void *data) {
	size_t id_value;

	pthread_mutex_lock(&(id_manager->mutex_list_released_ids));
		id_to_size(id_manager->id_data_type, &id_value, id_manager->id_counter);

		if(id_value <= (get_id_max(id_manager->id_data_type))) {
			// Si no se alcanzó el máximo de ID (sin fijarse si hay o no ID liberados), se asigna un nuevo ID
			pthread_mutex_unlock(&(id_manager->mutex_list_released_ids));

			pthread_mutex_lock(&(id_manager->mutex_cb_array));
				void **new_cb_array = realloc(id_manager->cb_array, sizeof(void *) * (id_value + 1));
				if(new_cb_array == NULL) {
					log_error(MODULE_LOGGER, "No se pudo reservar memoria para el array dinamico");
					exit(EXIT_FAILURE);
				}
				id_manager->cb_array = new_cb_array;

				(id_manager->cb_array)[id_value] = data;
			pthread_mutex_unlock(&(id_manager->mutex_cb_array));

			id_value++;
			size_to_id(id_manager->id_data_type, id_manager->id_counter, &id_value);

			return id_value;
		}

		// Se espera hasta que haya algún ID liberado para reutilizarlo
		while(id_manager->list_released_ids->head == NULL)
			pthread_cond_wait(&(id_manager->cond_list_released_ids), &(id_manager->mutex_list_released_ids));

		t_link_element *element = id_manager->list_released_ids->head;

		id_manager->list_released_ids->head = element->next;
		id_manager->list_released_ids->elements_count--;
	pthread_mutex_unlock(&(id_manager->mutex_list_released_ids));

	id_to_size(id_manager->id_data_type, &id_value, element->data);

	free(element->data);
	free(element);

	pthread_mutex_lock(&(id_manager->mutex_cb_array));
		(id_manager->cb_array)[id_value] = data;
	pthread_mutex_unlock(&(id_manager->mutex_cb_array));

	return id_value;
}

t_PID pid_assign(t_ID_Manager *id_manager, t_PCB *pcb) {
	return (t_PID) _id_assign(id_manager, (void *) pcb);
}

t_TID tid_assign(t_ID_Manager *id_manager, t_TCB *tcb) {
	return (t_TID) _id_assign(id_manager, (void *) tcb);
}

void _id_release(t_ID_Manager *id_manager, size_t id, void *data) {
	pthread_mutex_lock(&(id_manager->mutex_cb_array));
		(id_manager->cb_array)[id] = NULL;
	pthread_mutex_unlock(&(id_manager->mutex_cb_array));

	t_link_element *element = malloc(sizeof(t_link_element));
	if(element == NULL) {
		log_error(MODULE_LOGGER, "No se pudo reservar memoria para el elemento de la lista de IDs libres");
		exit(EXIT_FAILURE);
	}

	element->data = data;

	pthread_mutex_lock(&(id_manager->mutex_list_released_ids));
		element->next = id_manager->list_released_ids->head;
		id_manager->list_released_ids->head = element;
	pthread_mutex_unlock(&(id_manager->mutex_list_released_ids));

	pthread_cond_signal(&(id_manager->cond_list_released_ids));
}

void pid_release(t_ID_Manager *id_manager, t_PID pid) {
	t_PID *pid_copy = malloc(sizeof(t_PID));
	if(pid_copy == NULL) {
		log_error(MODULE_LOGGER, "No se pudo reservar memoria para el PID");
		exit(EXIT_FAILURE);
	}
	*pid_copy = pid;

	_id_release(id_manager, (size_t) pid, (void *) pid_copy);
}

void tid_release(t_ID_Manager *id_manager, t_TID tid) {
	t_TID *tid_copy = malloc(sizeof(t_TID));
	if(tid_copy == NULL) {
		log_error(MODULE_LOGGER, "No se pudo reservar memoria para el TID");
		exit(EXIT_FAILURE);
	}
	*tid_copy = tid;

	_id_release(id_manager, (size_t) tid, (void *) tid_copy);
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

void id_to_size(e_ID_Manager_Type id_data_type, size_t *destination, void *source) {
	switch(id_data_type) {
		case PROCESS_ID_MANAGER_TYPE:
			*destination = *((t_PID *) source);
		case THREAD_ID_MANAGER_TYPE:
			*destination = *((t_TID *) source);
	}
}

void size_to_id(e_ID_Manager_Type id_data_type, void *destination, size_t *source) {
	switch(id_data_type) {
		case PROCESS_ID_MANAGER_TYPE:
			*((t_PID *) destination) = (t_PID) (*source);
		case THREAD_ID_MANAGER_TYPE:
			*((t_TID *) destination) = (t_TID) (*source);
	}
}

bool pcb_matches_pid(t_PCB *pcb, t_PID *pid) {
	return pcb->PID == *pid;
}

bool tcb_matches_tid(t_TCB *tcb, t_TID *tid) {
	return tcb->TID == *tid;
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