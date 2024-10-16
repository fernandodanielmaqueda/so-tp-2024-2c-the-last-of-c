/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "kernel.h"

char *MODULE_NAME = "kernel";

t_log *MODULE_LOGGER = NULL;
char *MODULE_LOG_PATHNAME = "kernel.log";

t_config *MODULE_CONFIG = NULL;
char *MODULE_CONFIG_PATHNAME = "kernel.config";

t_PID_Manager PID_MANAGER;

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
	pthread_t THREAD_SIGNAL_MANAGER;
	if(pthread_create(&THREAD_SIGNAL_MANAGER, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main)) {
		perror("pthread_create");
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_SIGNAL_MANAGER);


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

	if((status = pthread_rwlock_init(&READY_RWLOCK, NULL))) {
		log_error_pthread_rwlock_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &READY_RWLOCK);

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


	// Long term scheduler
	if((status = pthread_create(&THREAD_LONG_TERM_SCHEDULER_NEW, NULL, (void *(*)(void *)) long_term_scheduler_new, NULL))) {
		log_error_pthread_create(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_LONG_TERM_SCHEDULER_NEW);

	if((status = pthread_create(&THREAD_LONG_TERM_SCHEDULER_EXIT, NULL, (void *(*)(void *)) long_term_scheduler_exit, NULL))) {
		log_error_pthread_create(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_LONG_TERM_SCHEDULER_EXIT);

	// Quantum Interrupter (Push)
	switch(SCHEDULING_ALGORITHM) {

		case FIFO_SCHEDULING_ALGORITHM:
		case PRIORITIES_SCHEDULING_ALGORITHM:
			goto skip_quantum_interrupter_push;

		case MLQ_SCHEDULING_ALGORITHM:
			break;
	}
	if((status = pthread_create(&THREAD_QUANTUM_INTERRUPTER, NULL, (void *(*)(void *)) quantum_interrupter, NULL))) {
		log_error_pthread_create(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_QUANTUM_INTERRUPTER);
	skip_quantum_interrupter_push:

	// IO Device
	if((status = pthread_create(&THREAD_IO_DEVICE, NULL, (void *(*)(void *)) io_device, NULL))) {
		log_error_pthread_create(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_IO_DEVICE);

	// Initial process
	if(new_process(process_size, argv[1], 0)) {
        log_error(MODULE_LOGGER, "No se pudo crear el proceso");
        pthread_exit(NULL);
    }
	// TODO


	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);


	/*
	SHARED_LIST_CONNECTIONS_MEMORY.list = list_create();
	if((status = pthread_mutex_init(&(SHARED_LIST_CONNECTIONS_MEMORY.mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		// TODO
	}
	*/

	// Short term scheduler
	short_term_scheduler(NULL);


	// Cleanup

	pthread_cleanup_pop(1); // THREAD_IO_DEVICE
	// Quantum Interrupter (Pop)
		switch(SCHEDULING_ALGORITHM) {

			case FIFO_SCHEDULING_ALGORITHM:
			case PRIORITIES_SCHEDULING_ALGORITHM:
				goto skip_quantum_interrupter_pop;

			case MLQ_SCHEDULING_ALGORITHM:
				break;
		}
		pthread_cleanup_pop(1); // THREAD_QUANTUM_INTERRUPTER
		skip_quantum_interrupter_pop:

	pthread_cleanup_pop(1); // THREAD_LONG_TERM_SCHEDULER_EXIT
	pthread_cleanup_pop(1); // THREAD_LONG_TERM_SCHEDULER_NEW
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
	pthread_cleanup_pop(1); // READY_RWLOCK
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
	int retval = 0;

	t_PCB *pcb = malloc(sizeof(t_PCB));
	if(pcb == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el PCB", sizeof(t_PCB));
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) free, pcb);

	pcb->size = size;

	if(tid_manager_init(&(pcb->thread_manager))) {
		retval = -1;
		goto cleanup_pcb;
	}
	pthread_cleanup_push((void (*)(void *)) tid_manager_destroy, &(pcb->thread_manager));

	pcb->dictionary_mutexes = dictionary_create();
	if(pcb->dictionary_mutexes == NULL) {
		retval = -1;
		goto cleanup_tid_manager;
	}
	pthread_cleanup_push((void (*)(void *)) dictionary_destroy, pcb->dictionary_mutexes);

	if(pid_assign(&PID_MANAGER, pcb, &(pcb->PID))) {
		retval = -1;
		goto cleanup_dictionary_mutexes;
	}

	cleanup_dictionary_mutexes:
		pthread_cleanup_pop(retval);
	cleanup_tid_manager:
		pthread_cleanup_pop(retval);
	cleanup_pcb:
		pthread_cleanup_pop(retval);
	ret:
		if(retval)
			return NULL;
		else
			return pcb;
}

int pcb_destroy(t_PCB *pcb) {
	int retval = 0;
	
	if(pid_release(&PID_MANAGER, pcb->PID)) {
		// TODO
		retval = -1;
	}

	dictionary_destroy(pcb->dictionary_mutexes);

	free(pcb);

	return retval;
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

int tcb_destroy(t_TCB *tcb) {
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

int pid_manager_init(t_PID_Manager *id_manager) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error(MODULE_LOGGER, "id_manager_init: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_init(&(id_manager->mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		retval = -1;
		goto ret;
	}

	id_manager->counter = 0;
	id_manager->array = NULL;

	ret:
		return retval;
}

int tid_manager_init(t_TID_Manager *id_manager) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error(MODULE_LOGGER, "id_manager_init: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_init(&(id_manager->mutex), NULL))) {
		log_error_pthread_mutex_init(status);
		retval = -1;
		goto ret;
	}

	id_manager->counter = 0;
	id_manager->array = NULL;

	ret:
		return retval;
}

int pid_manager_destroy(t_PID_Manager *id_manager) {
	int retval = 0, status;

	free(id_manager->array);
	if((status = pthread_mutex_destroy(&(id_manager->mutex)))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
		goto ret;
	}

	ret:
		return retval;
}

int tid_manager_destroy(t_TID_Manager *id_manager) {
	int retval = 0, status;

	free(id_manager->array);
	if((status = pthread_mutex_destroy(&(id_manager->mutex)))) {
		log_error_pthread_mutex_destroy(status);
		retval = -1;
		goto ret;
	}

	ret:
		return retval;
}

int pid_assign(t_PID_Manager *id_manager, t_PCB *data, t_PID *result) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error(MODULE_LOGGER, "id_assign: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if(id_manager->counter == PID_MAX) {
		log_error(MODULE_LOGGER, "id_assign: %s", strerror(ERANGE));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(id_manager->mutex));

		void **new_array = realloc(id_manager->array, sizeof(void *) * (id_manager->counter + 1));
		if(new_array == NULL) {
			log_error(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(void *) * id_manager->counter, sizeof(void *) * (id_manager->counter + 1));
			retval = -1;
		}
		id_manager->array = new_array;

		(id_manager->array)[id_manager->counter] = data;
		(id_manager->counter)++;

	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_unlock(status);
		retval = -1;
	}

	if(retval) {
		goto ret;
	}

	if(result != NULL) {
		memcpy(result, &(t_PID){id_manager->counter - 1}, sizeof(t_PID));
	}

	ret:
		return retval;
}

int tid_assign(t_TID_Manager *id_manager, t_TCB *data, t_TID *result) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error(MODULE_LOGGER, "id_assign: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if(id_manager->counter == TID_MAX) {
		log_error(MODULE_LOGGER, "id_assign: %s", strerror(ERANGE));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(id_manager->mutex));

		void **new_array = realloc(id_manager->array, sizeof(void *) * (id_manager->counter + 1));
		if(new_array == NULL) {
			log_error(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(void *) * id_manager->counter, sizeof(void *) * (id_manager->counter + 1));
			retval = -1;
		}
		id_manager->array = new_array;

		(id_manager->array)[id_manager->counter] = data;
		(id_manager->counter)++;

	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_unlock(status);
		retval = -1;
	}

	if(retval) {
		goto ret;
	}

	if(result != NULL) {
		memcpy(result, &(t_PID){id_manager->counter - 1}, sizeof(t_TID));
	}

	ret:
		return retval;
}

int pid_release(t_PID_Manager *id_manager, t_PID id) {
	int status;

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		(id_manager->array)[id] = NULL;
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	return 0;
}

int tid_release(t_TID_Manager *id_manager, t_TID id) {
	int status;

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		(id_manager->array)[id] = NULL;
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
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
	int retval = 0, status;

	t_PCB *pcb = pcb_create(size);
	if(pcb == NULL) {
		log_error(MODULE_LOGGER, "pcb_create: No se pudo crear el PCB");
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pcb_destroy, pcb);

	t_TCB *tcb = tcb_create(pcb, pseudocode_filename, priority);
	if(tcb == NULL) {
		log_error(MODULE_LOGGER, "tcb_create: No se pudo crear el TCB");
		retval = -1;
		goto cleanup_pcb;
	}
	pthread_cleanup_push((void (*)(void *)) tcb_destroy, tcb);

	if((status = pthread_mutex_lock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_lock(status);
		retval = -1;
		goto cleanup_tcb;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(SHARED_LIST_NEW.mutex));
		list_add(SHARED_LIST_NEW.list, pcb);
	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		retval = -1;
		goto cleanup_tcb;
	}

	log_info(MINIMAL_LOGGER, "## (<%u>:%u) Se crea el proceso - Estado: NEW", pcb->PID, tcb->TID);

	if(sem_post(&SEM_LONG_TERM_SCHEDULER_NEW)) {
		log_error_sem_post();
		retval = -1;
		goto cleanup_tcb;
	}

	return 0;

	cleanup_tcb:
		pthread_cleanup_pop(retval);
	cleanup_pcb:
		pthread_cleanup_pop(retval);
	ret:
		return retval;
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

	if((status = pthread_rwlock_wrlock(&READY_RWLOCK))) {
		log_error_pthread_rwlock_wrlock(status);
		return -1;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &READY_RWLOCK);

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
	
	pthread_cleanup_pop(0);
	if((status = pthread_rwlock_unlock(&READY_RWLOCK))) {
		log_error_pthread_rwlock_unlock(status);
		return -1;
	}

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