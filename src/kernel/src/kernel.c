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

	[BLOCKED_JOIN_STATE] = "BLOCKED",
	[BLOCKED_MUTEX_STATE] = "BLOCKED",
	[BLOCKED_DUMP_STATE] = "BLOCKED",
	[BLOCKED_IO_READY_STATE] = "BLOCKED",
	[BLOCKED_IO_EXEC_STATE] = "BLOCKED",

	[EXIT_STATE] = "EXIT"
};

char *SCHEDULING_ALGORITHMS[] = {
	[FIFO_SCHEDULING_ALGORITHM] = "FIFO",
	[PRIORITIES_SCHEDULING_ALGORITHM] = "PRIORIDADES",
	[MLQ_SCHEDULING_ALGORITHM] = "CMN"
};

e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

int module(int argc, char *argv[]) {
	int status;


	// Bloquea todas las señales para este y los hilos creados
	sigset_t set;
	if(sigfillset(&set)) {
		perror("sigfillset");
		return EXIT_FAILURE;
	}
	if((status = pthread_sigmask(SIG_BLOCK, &set, NULL))) {
		fprintf(stderr, "pthread_sigmask: %s\n", strerror(status));
		return EXIT_FAILURE;
	}


	// Verifica argumentos
	if(argc < 3) {
		fprintf(stderr, "Uso: %s <ARCHIVO_PSEUDOCODIGO> <TAMANIO_PROCESO> [ARGUMENTOS]\n", argv[0]);
		return EXIT_FAILURE;
	}

	size_t process_size;
	if(str_to_size(argv[2], &process_size)) {
		fprintf(stderr, "%s: No es un TAMANIO_PROCESO valido\n", argv[2]);
		return EXIT_FAILURE;
	}


	pthread_t thread_main = pthread_self();


	// Crea hilo para manejar señales
	pthread_t thread_signal_manager;
	if(pthread_create(&thread_signal_manager, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main)) {
		perror("pthread_create");
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &thread_signal_manager);


	// Config
	if((MODULE_CONFIG = config_create(MODULE_CONFIG_PATHNAME)) == NULL) {
		fprintf(stderr, "%s: No se pudo abrir el archivo de configuracion\n", MODULE_CONFIG_PATHNAME);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) config_destroy, MODULE_CONFIG);


	// Parse config
	if(read_module_config(MODULE_CONFIG)) {
		fprintf(stderr, "%s: El archivo de configuración no se pudo leer correctamente\n", MODULE_CONFIG_PATHNAME);
		pthread_exit(NULL);
	}


	// Loggers
	if(initialize_logger(&MINIMAL_LOGGER, MINIMAL_LOG_PATHNAME, "Minimal")) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MINIMAL_LOGGER);

	if(initialize_logger(&MODULE_LOGGER, MODULE_LOG_PATHNAME, MODULE_NAME)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MODULE_LOGGER);

	if(initialize_logger(&SOCKET_LOGGER, SOCKET_LOG_PATHNAME, "Socket")) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SOCKET_LOGGER);

	if(initialize_logger(&SERIALIZE_LOGGER, SERIALIZE_LOG_PATHNAME, "Serialize")) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SERIALIZE_LOGGER);


	// Variables globales
	if(shared_list_init(&SHARED_LIST_NEW)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) shared_list_destroy, (void *) &SHARED_LIST_NEW);

	if(resource_sync_init(&READY_SYNC)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) resource_sync_destroy, (void *) &READY_SYNC);

	pthread_cleanup_push((void (*)(void *)) array_list_ready_destroy, NULL);
	if(array_list_ready_init(0)) {
		pthread_exit(NULL);
	}

	if(shared_list_init(&SHARED_LIST_EXEC)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) shared_list_destroy, (void *) &SHARED_LIST_EXEC);

	if(shared_list_init(&SHARED_LIST_BLOCKED_MEMORY_DUMP)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) shared_list_destroy, (void *) &SHARED_LIST_BLOCKED_MEMORY_DUMP);

	if(shared_list_init(&SHARED_LIST_BLOCKED_IO_READY)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) shared_list_destroy, (void *) &SHARED_LIST_BLOCKED_IO_READY);

	if(shared_list_init(&SHARED_LIST_BLOCKED_IO_EXEC)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) shared_list_destroy, (void *) &SHARED_LIST_BLOCKED_IO_EXEC);

	if(shared_list_init(&SHARED_LIST_EXIT)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) shared_list_destroy, (void *) &SHARED_LIST_EXIT);

	if(sem_init(&SEM_LONG_TERM_SCHEDULER_NEW, 0, 0)) {
		log_error_sem_init();
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &SEM_LONG_TERM_SCHEDULER_NEW);

	if(sem_init(&SEM_LONG_TERM_SCHEDULER_EXIT, 0, 0)) {
		log_error_sem_init();
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &SEM_LONG_TERM_SCHEDULER_EXIT);

	if((status = pthread_mutex_init(&MUTEX_IS_TCB_IN_CPU, NULL))) {
		log_error_pthread_mutex_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_IS_TCB_IN_CPU);

	if((status = pthread_condattr_init(&CONDATTR_IS_TCB_IN_CPU))) {
		log_error_pthread_condattr_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_condattr_destroy, (void *) &CONDATTR_IS_TCB_IN_CPU);

	if((status = pthread_condattr_setclock(&CONDATTR_IS_TCB_IN_CPU, CLOCK_MONOTONIC))) {
		log_error_pthread_condattr_setclock(status);
		pthread_exit(NULL);
	}

	if((status = pthread_cond_init(&COND_IS_TCB_IN_CPU, &CONDATTR_IS_TCB_IN_CPU))) {
		log_error_pthread_cond_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_IS_TCB_IN_CPU);

	if(sem_init(&BINARY_QUANTUM_INTERRUPTER, 0, 0)) {
		log_error_sem_init();
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &BINARY_QUANTUM_INTERRUPTER);

	if(sem_init(&SEM_SHORT_TERM_SCHEDULER, 0, 0)) {
		log_error_sem_init();
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &SEM_SHORT_TERM_SCHEDULER);

	if(sem_init(&BINARY_SHORT_TERM_SCHEDULER, 0, 0)) {
		log_error_sem_init();
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &BINARY_SHORT_TERM_SCHEDULER);

	if((status = pthread_mutex_init(&MUTEX_FREE_MEMORY, NULL))) {
		log_error_pthread_mutex_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_FREE_MEMORY);

	if((status = pthread_cond_init(&COND_FREE_MEMORY, NULL))) {
		log_error_pthread_cond_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_FREE_MEMORY);


	// Sockets
	pthread_cleanup_push((void (*)(void *)) wrapper_close, (void *) &CONNECTION_CPU_DISPATCH.fd_connection);
	pthread_cleanup_push((void (*)(void *)) wrapper_close, (void *) &CONNECTION_CPU_INTERRUPT.fd_connection);
	initialize_sockets();


	if(initialize_long_term_scheduler()) {
		pthread_exit(NULL);
	}


	switch(SCHEDULING_ALGORITHM) {

		case FIFO_SCHEDULING_ALGORITHM:
		case PRIORITIES_SCHEDULING_ALGORITHM:
			break;

		case MLQ_SCHEDULING_ALGORITHM:
			if((status = pthread_create(&THREAD_QUANTUM_INTERRUPTER, NULL, (void *(*)(void *)) quantum_interrupter, NULL))) {
				log_error_pthread_create(status);
				pthread_exit(NULL);
			}
			break;
	}


	if((status = pthread_create(&THREAD_IO_DEVICE, NULL, (void *(*)(void *)) io_device, NULL))) {
		log_error_pthread_create(status);
		pthread_exit(NULL);
	}


	if(new_process(process_size, argv[1], 0)) {
        log_error(MODULE_LOGGER, "No se pudo crear el proceso");
        pthread_exit(NULL);
    }


	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);


	/*
	SHARED_LIST_CONNECTIONS_MEMORY.list = list_create();
	if((status = pthread_mutex_init(&(SHARED_LIST_CONNECTIONS_MEMORY.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		// TODO
	}
	*/


	short_term_scheduler(NULL);

	/*
	error_io_device:
		cancel_and_join_pthread(&THREAD_IO_DEVICE);
	error_quantum_interrupter:
		switch(SCHEDULING_ALGORITHM) {

			case FIFO_SCHEDULING_ALGORITHM:
			case PRIORITIES_SCHEDULING_ALGORITHM:
				break;

			case MLQ_SCHEDULING_ALGORITHM:
				cancel_and_join_pthread(&THREAD_QUANTUM_INTERRUPTER);
				break;
		}
	error_long_term_scheduler:
		finish_long_term_scheduler();
	error_sockets:
		finish_sockets();
	*/
		
	pthread_cleanup_pop(1); // CONNECTION_CPU_INTERRUPT
	pthread_cleanup_pop(1); // CONNECTION_CPU_DISPATCH
	pthread_cleanup_pop(1); // COND_FREE_MEMORY
	pthread_cleanup_pop(1); // MUTEX_FREE_MEMORY
	pthread_cleanup_pop(1); // BINARY_SHORT_TERM_SCHEDULER
	pthread_cleanup_pop(1); // SEM_SHORT_TERM_SCHEDULER
	pthread_cleanup_pop(1); // BINARY_QUANTUM_INTERRUPTER
	pthread_cleanup_pop(1); // COND_IS_TCB_IN_CPU
	pthread_cleanup_pop(1); // CONDATTR_IS_TCB_IN_CPU
	pthread_cleanup_pop(1); // MUTEX_IS_TCB_IN_CPU
	pthread_cleanup_pop(1); // SEM_LONG_TERM_SCHEDULER_EXIT
	pthread_cleanup_pop(1); // SEM_LONG_TERM_SCHEDULER_NEW
	pthread_cleanup_pop(1); // SHARED_LIST_EXIT
	pthread_cleanup_pop(1); // SHARED_LIST_BLOCKED_IO_EXEC
	pthread_cleanup_pop(1); // SHARED_LIST_BLOCKED_IO_READY
	pthread_cleanup_pop(1); // SHARED_LIST_BLOCKED_MEMORY_DUMP
	pthread_cleanup_pop(1); // SHARED_LIST_EXEC
	pthread_cleanup_pop(1); // ARRAY_LIST_READY
	pthread_cleanup_pop(1); // READY_SYNC
	pthread_cleanup_pop(1); // SHARED_LIST_NEW
	pthread_cleanup_pop(1); // SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // SOCKET_LOGGER
	pthread_cleanup_pop(1); // MODULE_LOGGER
	pthread_cleanup_pop(1); // MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MODULE_CONFIG
	pthread_cleanup_pop(1); // thread_signal_manager

	return EXIT_SUCCESS;
}

int read_module_config(t_config *module_config) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "IP_CPU", "PUERTO_CPU_DISPATCH", "PUERTO_CPU_INTERRUPT", "ALGORITMO_PLANIFICACION", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

	CONNECTION_CPU_DISPATCH = (t_Connection) {.fd_connection = -1, .client_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .server_type = CPU_DISPATCH_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_DISPATCH")};
	CONNECTION_CPU_INTERRUPT = (t_Connection) {.fd_connection = -1, .client_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .server_type = CPU_INTERRUPT_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_INTERRUPT")};

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

int array_list_ready_init(t_Priority priority) {
	int status;

	// Si la lista de READY ya fue creada, retorna inmediatamente
	if(priority < PRIORITY_COUNT) {
		return 0;
	}

	// Valida que no se produzca un overflow por el tamaño en bytes o por la cantidad de elementos del array
	if(priority >= PRIORITY_LIMIT) {
		log_error(MODULE_LOGGER, "array_list_ready_init: %s", strerror(ERANGE));
		errno = ERANGE;
		return -1;
	}

	// TODO: INICIO SECCIÓN CRÍTICA

		t_Shared_List *new_array_list_ready = realloc(ARRAY_LIST_READY, sizeof(t_Shared_List) * (priority + 1));
		if(new_array_list_ready == NULL) {
			log_error(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(t_Shared_List) * PRIORITY_COUNT, sizeof(t_Shared_List) * (priority + 1));
			errno = ENOMEM;
			return -1;
		}
		ARRAY_LIST_READY = new_array_list_ready;
		
		for(t_Priority i = PRIORITY_COUNT; i <= priority; i++) {
			if(shared_list_init(&(ARRAY_LIST_READY[i]))) {

				// Si una de las inicializaciones falla, Se trunca el array para sólo incluir las listas de READY que se pudieron inicializar
				new_array_list_ready = realloc(ARRAY_LIST_READY, sizeof(t_Shared_List) * i);
				if(new_array_list_ready == NULL) {
					log_error(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(t_Shared_List) * PRIORITY_COUNT, sizeof(t_Shared_List) * i);
				}
				else {
					ARRAY_LIST_READY = new_array_list_ready;
					PRIORITY_COUNT = i;
				}

				return -1;
			}
		}

		PRIORITY_COUNT = priority + 1;
	
	// TODO: FIN SECCIÓN CRÍTICA

	return 0;

}

int array_list_ready_destroy(void *NULL_parameter) {
	int retval = 0;
	for(t_Priority i = 0; i < PRIORITY_COUNT; i++) {
		if(shared_list_destroy(&(ARRAY_LIST_READY[PRIORITY_COUNT - 1 - i]))) {
			retval = -1;
		}
	}
	free(ARRAY_LIST_READY);
	PRIORITY_COUNT = 0;
	return retval;
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