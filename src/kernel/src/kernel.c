/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "kernel.h"

t_PID_Manager PID_MANAGER;

const char *STATE_NAMES[] = {
	[NEW_STATE] = "NEW",
	[READY_STATE] = "READY",
	[EXEC_STATE] = "EXEC",

	[BLOCKED_JOIN_STATE] = "BLOCKED_JOIN",
	[BLOCKED_MUTEX_STATE] = "BLOCKED_MUTEX",
	[BLOCKED_DUMP_STATE] = "BLOCKED_DUMP",
	[BLOCKED_IO_READY_STATE] = "BLOCKED_IO_READY",
	[BLOCKED_IO_EXEC_STATE] = "BLOCKED_IO_EXEC",

	[EXIT_STATE] = "EXIT"
};

const char *EXIT_REASONS[] = {
	[UNEXPECTED_ERROR_EXIT_REASON] = "UNEXPECTED_ERROR",

	[SEGMENTATION_FAULT_EXIT_REASON] = "SEGMENTATION_FAULT",
	[PROCESS_EXIT_EXIT_REASON] = "PROCESS_EXIT",
	[THREAD_EXIT_EXIT_REASON] = "THREAD_EXIT",
	[THREAD_CANCEL_EXIT_REASON] = "THREAD_CANCEL",
	[DUMP_MEMORY_ERROR_EXIT_REASON] = "DUMP_MEMORY_ERROR",
	[INVALID_RESOURCE_EXIT_REASON] = "INVALID_RESOURCE"
};

char *SCHEDULING_ALGORITHMS[] = {
	[FIFO_SCHEDULING_ALGORITHM] = "FIFO",
	[PRIORITIES_SCHEDULING_ALGORITHM] = "PRIORIDADES",
	[MLQ_SCHEDULING_ALGORITHM] = "CMN"
};

e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

int module(int argc, char *argv[]) {
	int status;


	MODULE_NAME = "kernel";
	MODULE_CONFIG_PATHNAME = "kernel.config";


	// Bloquea todas las señales para este y los hilos creados
	sigset_t set;
	if(sigfillset(&set)) {
		report_error_sigfillset();
		return EXIT_FAILURE;
	}
	if((status = pthread_sigmask(SIG_BLOCK, &set, NULL))) {
		report_error_pthread_sigmask(status);
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
	if((status = pthread_create(&THREAD_SIGNAL_MANAGER, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main))) {
		report_error_pthread_create(status);
		return EXIT_FAILURE;
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_SIGNAL_MANAGER);


	// Config
	if((MODULE_CONFIG = config_create(MODULE_CONFIG_PATHNAME)) == NULL) {
		fprintf(stderr, "%s: No se pudo abrir el archivo de configuracion\n", MODULE_CONFIG_PATHNAME);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) config_destroy, MODULE_CONFIG);


	// Parse config
	if(read_module_config(MODULE_CONFIG)) {
		fprintf(stderr, "%s: El archivo de configuración no se pudo leer correctamente\n", MODULE_CONFIG_PATHNAME);
		exit_sigint();
	}

	// Loggers
	if((status = pthread_mutex_init(&MUTEX_LOGGERS, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_LOGGERS);

	if(logger_init(&MODULE_LOGGER, MODULE_LOGGER_INIT_ENABLED, MODULE_LOGGER_PATHNAME, MODULE_LOGGER_NAME, MODULE_LOGGER_INIT_ACTIVE_CONSOLE, MODULE_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &MODULE_LOGGER);

	if(logger_init(&MINIMAL_LOGGER, MINIMAL_LOGGER_INIT_ENABLED, MINIMAL_LOGGER_PATHNAME, MINIMAL_LOGGER_NAME, MINIMAL_LOGGER_ACTIVE_CONSOLE, MINIMAL_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &MINIMAL_LOGGER);

	if(logger_init(&SOCKET_LOGGER, SOCKET_LOGGER_INIT_ENABLED, SOCKET_LOGGER_PATHNAME, SOCKET_LOGGER_NAME, SOCKET_LOGGER_INIT_ACTIVE_CONSOLE, SOCKET_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &SOCKET_LOGGER);

	if(logger_init(&SERIALIZE_LOGGER, SERIALIZE_LOGGER_INIT_ENABLED, SERIALIZE_LOGGER_PATHNAME, SERIALIZE_LOGGER_NAME, SERIALIZE_LOGGER_INIT_ACTIVE_CONSOLE, SERIALIZE_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &SERIALIZE_LOGGER);


	// PID_MANAGER
	if(pid_manager_init(&PID_MANAGER)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pid_manager_destroy, (void *) &PID_MANAGER);


	// RWLOCK_SCHEDULING
	if((status = pthread_rwlock_init(&RWLOCK_SCHEDULING, NULL))) {
		report_error_pthread_rwlock_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &RWLOCK_SCHEDULING);

	// SHARED_LIST_NEW
	if((status = pthread_mutex_init(&(SHARED_LIST_NEW.mutex), NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_NEW.mutex));

	SHARED_LIST_NEW.list = list_create();
	if(SHARED_LIST_NEW.list == NULL) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) list_destroy, (void *) SHARED_LIST_NEW.list);

	// RWLOCK_ARRAY_READY
	if((status = pthread_rwlock_init(&RWLOCK_ARRAY_READY, NULL))) {
		report_error_pthread_rwlock_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &RWLOCK_ARRAY_READY);

	// ARRAY_LIST_READY
	if(array_list_ready_init()) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) array_list_ready_destroy, NULL);

	// MUTEX_EXEC
	if((status = pthread_mutex_init(&MUTEX_EXEC, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_EXEC);

	// SHARED_LIST_BLOCKED_DUMP_MEMORY
	if((status = pthread_mutex_init(&(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex), NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex));

	SHARED_LIST_BLOCKED_DUMP_MEMORY.list = list_create();
	if(SHARED_LIST_BLOCKED_DUMP_MEMORY.list == NULL) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) list_destroy, (void *) SHARED_LIST_BLOCKED_DUMP_MEMORY.list);

	// COND_BLOCKED_DUMP_MEMORY
	if((status = pthread_cond_init(&COND_BLOCKED_DUMP_MEMORY, NULL))) {
		report_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_BLOCKED_DUMP_MEMORY);

	// SHARED_LIST_BLOCKED_IO_READY
	if((status = pthread_mutex_init(&(SHARED_LIST_BLOCKED_IO_READY.mutex), NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_BLOCKED_IO_READY.mutex));

	SHARED_LIST_BLOCKED_IO_READY.list = list_create();
	if(SHARED_LIST_BLOCKED_IO_READY.list == NULL) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) list_destroy, (void *) SHARED_LIST_BLOCKED_IO_READY.list);

	// MUTEX_BLOCKED_IO_EXEC
	if((status = pthread_mutex_init(&MUTEX_BLOCKED_IO_EXEC, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_BLOCKED_IO_EXEC);

	// SHARED_LIST_EXIT
	if((status = pthread_mutex_init(&(SHARED_LIST_EXIT.mutex), NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_EXIT.mutex));

	SHARED_LIST_EXIT.list = list_create();
	if(SHARED_LIST_EXIT.list == NULL) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) list_destroy, (void *) SHARED_LIST_EXIT.list);

	if(sem_init(&SEM_LONG_TERM_SCHEDULER_NEW, 0, 0)) {
		report_error_sem_init();
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &SEM_LONG_TERM_SCHEDULER_NEW);

	if(sem_init(&SEM_LONG_TERM_SCHEDULER_EXIT, 0, 0)) {
		report_error_sem_init();
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &SEM_LONG_TERM_SCHEDULER_EXIT);

	if((status = pthread_mutex_init(&MUTEX_IS_TCB_IN_CPU, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_IS_TCB_IN_CPU);

	if((status = pthread_condattr_init(&CONDATTR_IS_TCB_IN_CPU))) {
		report_error_pthread_condattr_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_condattr_destroy, (void *) &CONDATTR_IS_TCB_IN_CPU);

	if((status = pthread_condattr_setclock(&CONDATTR_IS_TCB_IN_CPU, CLOCK_MONOTONIC))) {
		report_error_pthread_condattr_setclock(status);
		exit_sigint();
	}

	if((status = pthread_cond_init(&COND_IS_TCB_IN_CPU, &CONDATTR_IS_TCB_IN_CPU))) {
		report_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_IS_TCB_IN_CPU);

	if(sem_init(&BINARY_QUANTUM_INTERRUPTER, 0, 0)) {
		report_error_sem_init();
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &BINARY_QUANTUM_INTERRUPTER);

	if((status = pthread_cond_init(&COND_QUANTUM_INTERRUPTER, NULL))) {
		report_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_QUANTUM_INTERRUPTER);

	if(sem_init(&SEM_SHORT_TERM_SCHEDULER, 0, 0)) {
		report_error_sem_init();
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &SEM_SHORT_TERM_SCHEDULER);

	if(sem_init(&BINARY_SHORT_TERM_SCHEDULER, 0, 0)) {
		report_error_sem_init();
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &BINARY_SHORT_TERM_SCHEDULER);


	if((status = pthread_mutex_init(&MUTEX_CANCEL_IO_OPERATION, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_CANCEL_IO_OPERATION);

	if((status = pthread_condattr_init(&CONDATTR_CANCEL_IO_OPERATION))) {
		report_error_pthread_condattr_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_condattr_destroy, (void *) &CONDATTR_CANCEL_IO_OPERATION);

	if((status = pthread_condattr_setclock(&CONDATTR_CANCEL_IO_OPERATION, CLOCK_MONOTONIC))) {
		report_error_pthread_condattr_setclock(status);
		exit_sigint();
	}

	if((status = pthread_cond_init(&COND_CANCEL_IO_OPERATION, &CONDATTR_CANCEL_IO_OPERATION))) {
		report_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_CANCEL_IO_OPERATION);


	if(sem_init(&SEM_IO_DEVICE, 0, 0)) {
		report_error_sem_init();
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) sem_destroy, (void *) &SEM_IO_DEVICE);


	if((status = pthread_mutex_init(&MUTEX_FREE_MEMORY, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_FREE_MEMORY);

	if((status = pthread_cond_init(&COND_FREE_MEMORY, NULL))) {
		report_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_FREE_MEMORY);


	// Sockets
	pthread_cleanup_push((void (*)(void *)) finish_sockets, NULL);
	initialize_sockets();


	// wait_dump_memory_threads
	pthread_cleanup_push((void (*)(void *)) wait_dump_memory_threads, NULL);


	// Scheduling
	pthread_cleanup_push((void (*)(void *)) finish_scheduling, NULL);
	initialize_scheduling();


	// Initial process
	char *pseudocode_filename = strdup(argv[1]);
	if(pseudocode_filename == NULL) {
		log_error_r(&MODULE_LOGGER, "strdup: No se pudo duplicar el nombre del archivo de pseudocodigo");
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) free, pseudocode_filename);
		if(new_process(process_size, pseudocode_filename, 0)) {
			log_error_r(&MODULE_LOGGER, "No se pudo crear el proceso");
			exit_sigint();
		}
	pthread_cleanup_pop(0); // pseudocode_filename


	log_debug_r(&MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);


	// Short term scheduler
	short_term_scheduler();


	// Cleanup

	pthread_cleanup_pop(1); // Scheduling
	pthread_cleanup_pop(1); // wait_dump_memory_threads
	pthread_cleanup_pop(1); // Sockets
	pthread_cleanup_pop(1); // COND_FREE_MEMORY
	pthread_cleanup_pop(1); // MUTEX_FREE_MEMORY
	pthread_cleanup_pop(1); // SEM_IO_DEVICE
	pthread_cleanup_pop(1); // COND_CANCEL_IO_OPERATION
	pthread_cleanup_pop(1); // CONDATTR_CANCEL_IO_OPERATION
	pthread_cleanup_pop(1); // MUTEX_CANCEL_IO_OPERATION
	pthread_cleanup_pop(1); // BINARY_SHORT_TERM_SCHEDULER
	pthread_cleanup_pop(1); // SEM_SHORT_TERM_SCHEDULER
	pthread_cleanup_pop(1); // COND_QUANTUM_INTERRUPTER
	pthread_cleanup_pop(1); // BINARY_QUANTUM_INTERRUPTER
	pthread_cleanup_pop(1); // COND_IS_TCB_IN_CPU
	pthread_cleanup_pop(1); // CONDATTR_IS_TCB_IN_CPU
	pthread_cleanup_pop(1); // MUTEX_IS_TCB_IN_CPU
	pthread_cleanup_pop(1); // SEM_LONG_TERM_SCHEDULER_EXIT
	pthread_cleanup_pop(1); // SEM_LONG_TERM_SCHEDULER_NEW
	pthread_cleanup_pop(1); // LIST_EXIT
	pthread_cleanup_pop(1); // MUTEX_EXIT
	pthread_cleanup_pop(1); // MUTEX_BLOCKED_IO_EXEC
	pthread_cleanup_pop(1); // LIST_BLOCKED_IO_READY
	pthread_cleanup_pop(1); // MUTEX_BLOCKED_IO_READY
	pthread_cleanup_pop(1); // COND_BLOCKED_DUMP_MEMORY
	pthread_cleanup_pop(1); // LIST_BLOCKED_DUMP_MEMORY
	pthread_cleanup_pop(1); // MUTEX_BLOCKED_DUMP_MEMORY
	pthread_cleanup_pop(1); // MUTEX_EXEC
	pthread_cleanup_pop(1); // ARRAY_LIST_READY
	pthread_cleanup_pop(1); // RWLOCK_ARRAY_READY
	pthread_cleanup_pop(1); // LIST_NEW
	pthread_cleanup_pop(1); // MUTEX_NEW
	pthread_cleanup_pop(1); // RWLOCK_SCHEDULING
	pthread_cleanup_pop(1); // PID_MANAGER
	pthread_cleanup_pop(1); // SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // SOCKET_LOGGER
	pthread_cleanup_pop(1); // MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MODULE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_LOGGERS
	pthread_cleanup_pop(1); // MODULE_CONFIG
	pthread_cleanup_pop(1); // THREAD_SIGNAL_MANAGER

	return EXIT_SUCCESS;
}

int read_module_config(t_config *module_config) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "IP_CPU", "PUERTO_CPU_DISPATCH", "PUERTO_CPU_INTERRUPT", "ALGORITMO_PLANIFICACION", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

	CONNECTION_CPU_DISPATCH = (t_Connection) {.socket_connection.fd = -1, .socket_connection.bool_thread.running = false, .client_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .server_type = CPU_DISPATCH_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_DISPATCH")};
	CONNECTION_CPU_INTERRUPT = (t_Connection) {.socket_connection.fd = -1, .socket_connection.bool_thread.running = false, .client_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .server_type = CPU_INTERRUPT_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_INTERRUPT")};

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
	int retval = 0, status;

	t_PCB *pcb = malloc(sizeof(t_PCB));
	if(pcb == NULL) {
		log_error_r(&MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el PCB", sizeof(t_PCB));
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

	if((status = pthread_rwlock_init(&(pcb->rwlock_resources), NULL))) {
		report_error_pthread_rwlock_init(status);
		retval = -1;
		goto cleanup_tid_manager;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &(pcb->rwlock_resources));

	pcb->dictionary_resources = dictionary_create();
	if(pcb->dictionary_resources == NULL) {
		retval = -1;
		goto cleanup_rwlock_resources;
	}
	pthread_cleanup_push((void (*)(void *)) dictionary_destroy, pcb->dictionary_resources);

	if(pid_assign(&PID_MANAGER, pcb, &(pcb->PID))) {
		retval = -1;
		goto cleanup_dictionary_resources;
	}

	cleanup_dictionary_resources:
	pthread_cleanup_pop(retval);

	cleanup_rwlock_resources:
	pthread_cleanup_pop(0);
	if(retval) {
		if((status = pthread_rwlock_destroy(&(pcb->rwlock_resources)))) {
			report_error_pthread_rwlock_destroy(status);
		}
	}

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
	int retval = 0, status;
	
	if(pid_release(&PID_MANAGER, pcb->PID)) {
		retval = -1;
	}

	dictionary_destroy(pcb->dictionary_resources);

	if((status = pthread_rwlock_destroy(&(pcb->rwlock_resources)))) {
		report_error_pthread_rwlock_destroy(status);
		retval = -1;
	}

	if(tid_manager_destroy(&(pcb->thread_manager))) {
		retval = -1;
	}

	free(pcb);

	return retval;
}

t_TCB *tcb_create(t_PCB *pcb, char *pseudocode_filename, t_Priority priority) {
	int retval = 0, status;

	t_TCB *tcb = malloc(sizeof(t_TCB));
	if(tcb == NULL) {
		log_error_r(&MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el TCB", sizeof(t_TCB));
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) free, tcb);

	tcb->pcb = pcb;

	tcb->pseudocode_filename = pseudocode_filename;
	tcb->priority = priority;

	tcb->current_state = NEW_STATE;
	tcb->location = NULL;

	tcb->quantum = QUANTUM;

	payload_init(&(tcb->syscall_instruction));

	if((status = pthread_mutex_init(&(tcb->shared_list_blocked_thread_join.mutex), NULL))) {
		report_error_pthread_mutex_init(status);
		retval = -1;
		goto cleanup_tcb;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(tcb->shared_list_blocked_thread_join.mutex));

	tcb->shared_list_blocked_thread_join.list = list_create();
	if(tcb->shared_list_blocked_thread_join.list == NULL) {
		retval = -1;
		goto cleanup_mutex_blocked_thread_join;
	}
	pthread_cleanup_push((void (*)(void *)) list_destroy, (void *) tcb->shared_list_blocked_thread_join.list);

	tcb->dictionary_assigned_resources = dictionary_create();
	if(tcb->dictionary_assigned_resources == NULL) {
		retval = -1;
		goto cleanup_list_blocked_thread_join;
	}
	pthread_cleanup_push((void (*)(void *)) dictionary_destroy, tcb->dictionary_assigned_resources);

	tcb->exit_reason = UNEXPECTED_ERROR_EXIT_REASON;

	if(tid_assign(&(pcb->thread_manager), tcb, &(tcb->TID))) {
		retval = -1;
		goto cleanup_dictionary_assigned_resources;
	}

	cleanup_dictionary_assigned_resources:
	pthread_cleanup_pop(retval);

	cleanup_list_blocked_thread_join:
	pthread_cleanup_pop(retval);

	cleanup_mutex_blocked_thread_join:
	pthread_cleanup_pop(0);
	if(retval) {
		if((status = pthread_mutex_destroy(&(tcb->shared_list_blocked_thread_join.mutex)))) {
			report_error_pthread_mutex_destroy(status);
		}
	}

	cleanup_tcb:
	pthread_cleanup_pop(retval);

	ret:
	if(retval)
		return NULL;
	else
		return tcb;
}

int tcb_destroy(t_TCB *tcb) {
	int retval = 0, status;

	if(tid_release(&(tcb->pcb->thread_manager), tcb->TID)) {
		retval = -1;
	}

	dictionary_destroy(tcb->dictionary_assigned_resources);

	list_destroy(tcb->shared_list_blocked_thread_join.list);

	if((status = pthread_mutex_destroy(&(tcb->shared_list_blocked_thread_join.mutex)))) {
		report_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	payload_destroy(&(tcb->syscall_instruction));

	free(tcb->pseudocode_filename);

	free(tcb);

	return retval;
}

int pid_manager_init(t_PID_Manager *id_manager) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error_r(&MODULE_LOGGER, "id_manager_init: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_init(&(id_manager->mutex), NULL))) {
		report_error_pthread_mutex_init(status);
		retval = -1;
		goto ret;
	}

	id_manager->size = 0;
	id_manager->array = NULL;

	id_manager->counter = 0;

	ret:
	return retval;
}

int tid_manager_init(t_TID_Manager *id_manager) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error_r(&MODULE_LOGGER, "id_manager_init: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_init(&(id_manager->mutex), NULL))) {
		report_error_pthread_mutex_init(status);
		retval = -1;
		goto ret;
	}

	id_manager->size = 0;
	id_manager->array = NULL;

	id_manager->counter = 0;

	ret:
	return retval;
}

int pid_manager_destroy(t_PID_Manager *id_manager) {
	int retval = 0, status;

	for(register t_PID pid = 0; pid < id_manager->size; pid++) {
		if((id_manager->array)[pid] != NULL) {
			if(pcb_destroy((id_manager->array)[pid])) {
				retval = -1;
			}
		}
	}

	free(id_manager->array);

	if((status = pthread_mutex_destroy(&(id_manager->mutex)))) {
		report_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	return retval;
}

int tid_manager_destroy(t_TID_Manager *id_manager) {
	int retval = 0, status;

	for(register t_TID tid = 0; tid < id_manager->size; tid++) {
		if((id_manager->array)[tid] != NULL) {
			if(tcb_destroy((id_manager->array)[tid])) {
				retval = -1;
			}
		}
	}

	free(id_manager->array);

	if((status = pthread_mutex_destroy(&(id_manager->mutex)))) {
		report_error_pthread_mutex_destroy(status);
		retval = -1;
	}

	return retval;
}

int pid_assign(t_PID_Manager *id_manager, t_PCB *data, t_PID *result) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error_r(&MODULE_LOGGER, "id_assign: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if(id_manager->size == PID_MAX) {
		log_error_r(&MODULE_LOGGER, "id_assign: %s", strerror(ERANGE));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(id_manager->mutex));

		void **new_array = realloc(id_manager->array, sizeof(void *) * (id_manager->size + 1));
		if(new_array == NULL) {
			log_error_r(&MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(void *) * id_manager->size, sizeof(void *) * (id_manager->size + 1));
			retval = -1;
		}
		id_manager->array = new_array;

		(id_manager->array)[id_manager->size] = data;
		(id_manager->size)++;

		(id_manager->counter)++;

	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_unlock(status);
		retval = -1;
	}

	if(retval) {
		goto ret;
	}

	if(result != NULL) {
		memcpy(result, &(t_PID){id_manager->size - 1}, sizeof(t_PID));
	}

	ret:
	return retval;
}

int tid_assign(t_TID_Manager *id_manager, t_TCB *data, t_TID *result) {
	int retval = 0, status;

	if(id_manager == NULL) {
		log_error_r(&MODULE_LOGGER, "id_assign: %s", strerror(EINVAL));
		retval = -1;
		goto ret;
	}

	if(id_manager->size == TID_MAX) {
		log_error_r(&MODULE_LOGGER, "id_assign: %s", strerror(ERANGE));
		retval = -1;
		goto ret;
	}

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(id_manager->mutex));

		void **new_array = realloc(id_manager->array, sizeof(void *) * (id_manager->size + 1));
		if(new_array == NULL) {
			log_error_r(&MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(void *) * id_manager->size, sizeof(void *) * (id_manager->size + 1));
			retval = -1;
		}
		id_manager->array = new_array;

		(id_manager->array)[id_manager->size] = data;
		(id_manager->size)++;

		(id_manager->counter)++;

	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_unlock(status);
		retval = -1;
	}

	if(retval) {
		goto ret;
	}

	if(result != NULL) {
		memcpy(result, &(t_PID){id_manager->size - 1}, sizeof(t_TID));
	}

	ret:
	return retval;
}

int pid_release(t_PID_Manager *id_manager, t_PID id) {
	int status;

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_lock(status);
		return -1;
	}
		(id_manager->array)[id] = NULL;
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_unlock(status);
		return -1;
	}

	(id_manager->counter)--;

	return 0;
}

int tid_release(t_TID_Manager *id_manager, t_TID id) {
	int status;

	if((status = pthread_mutex_lock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_lock(status);
		return -1;
	}
		(id_manager->array)[id] = NULL;
	if((status = pthread_mutex_unlock(&(id_manager->mutex)))) {
		report_error_pthread_mutex_unlock(status);
		return -1;
	}

	(id_manager->counter)--;

	return 0;
}

bool pcb_matches_pid(t_PCB *pcb, t_PID *pid) {
	return pcb->PID == *pid;
}

bool tcb_matches_tid(t_TCB *tcb, t_TID *tid) {
	return tcb->TID == *tid;
}

int new_process(size_t size, char *pseudocode_filename, t_Priority priority) {
	int retval = 0;

	t_PCB *pcb = pcb_create(size);
	if(pcb == NULL) {
		log_error_r(&MODULE_LOGGER, "pcb_create: No se pudo crear el PCB");
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pcb_destroy, pcb);

	t_TCB *tcb = tcb_create(pcb, pseudocode_filename, priority);
	if(tcb == NULL) {
		log_error_r(&MODULE_LOGGER, "tcb_create: No se pudo crear el TCB");
		retval = -1;
		goto cleanup_pcb;
	}
	pthread_cleanup_push((void (*)(void *)) tcb_destroy, tcb);

	if(insert_state_new(pcb)) {
		retval = -1;
		goto cleanup_tcb;
	}

	cleanup_tcb:
	pthread_cleanup_pop(retval); // tcb_destroy
	cleanup_pcb:
	pthread_cleanup_pop(retval); // pcb_destroy
	ret:
	return retval;
}

int request_thread_create(t_PCB *pcb, t_TID tid, int *result) {
	int retval = 0;

	t_Connection connection_memory = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};

	client_thread_connect_to_server(&connection_memory);
	pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_memory.socket_connection.fd));

		if(send_thread_create(pcb->PID, tid, ((t_TCB **) (pcb->thread_manager.array))[tid]->pseudocode_filename, connection_memory.socket_connection.fd)) {
			log_error_r(&MODULE_LOGGER, "[%d] Error al enviar solicitud de creación de hilo a [Servidor] %s [PID: %u - TID: %u - Archivo: %s]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID, tid, ((t_TCB **) (pcb->thread_manager.array))[tid]->pseudocode_filename);
			retval = -1;
			goto cleanup_connection;
		}
		log_trace_r(&MODULE_LOGGER, "[%d] Se envía solicitud de creación de hilo a [Servidor] %s [PID: %u - TID: %u - Archivo: %s]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID, tid, ((t_TCB **) (pcb->thread_manager.array))[tid]->pseudocode_filename);

		if(receive_result_with_expected_header(THREAD_CREATE_HEADER, result, connection_memory.socket_connection.fd)) {
			log_error_r(&MODULE_LOGGER, "[%d] Error al recibir resultado de creación de hilo de [Servidor] %s [PID: %u - TID: %u]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID, tid);
			retval = -1;
			goto cleanup_connection;
		}
		log_trace_r(&MODULE_LOGGER, "[%d] Se recibe resultado de creación de hilo de [Servidor] %s [PID: %u - TID: %u - Resultado: %d]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID, tid, *result);

	cleanup_connection:
	pthread_cleanup_pop(0);
	if(close(connection_memory.socket_connection.fd)) {
		report_error_close();
		return -1;
	}

	return retval;
}

int array_list_ready_init(void) {
	// Todos los algoritmos de planificación requieren la lista de READY 0
	return array_list_ready_resize(0);
}

int array_list_ready_update(t_Priority priority) {
	switch(SCHEDULING_ALGORITHM) {

		case FIFO_SCHEDULING_ALGORITHM:
			break;

		case PRIORITIES_SCHEDULING_ALGORITHM:
		case MLQ_SCHEDULING_ALGORITHM:
			if(array_list_ready_resize(priority)) {
				return -1;
			}
			break;
	}

	return 0;
}

int array_list_ready_resize(t_Priority priority) {
	int status;

	// Si la lista de READY ya fue creada, retorna inmediatamente
	if(priority < PRIORITY_COUNT) {
		return 0;
	}

	// Valida que no se produzca un overflow por el tamaño en bytes o por la cantidad de elementos del array
	if(priority >= PRIORITY_LIMIT) {
		log_error_r(&MODULE_LOGGER, "array_list_ready_resize: %s", strerror(ERANGE));
		errno = ERANGE;
		return -1;
	}

	if((status = pthread_rwlock_wrlock(&RWLOCK_ARRAY_READY))) {
		report_error_pthread_rwlock_wrlock(status);
		return -1;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_ARRAY_READY);

		t_Shared_List *new_array_list_ready = realloc(ARRAY_LIST_READY, sizeof(t_Shared_List) * (priority + 1));
		if(new_array_list_ready == NULL) {
			log_error_r(&MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(t_Shared_List) * PRIORITY_COUNT, sizeof(t_Shared_List) * (priority + 1));
			errno = ENOMEM;
			return -1;
		}
		ARRAY_LIST_READY = new_array_list_ready;

		for(t_Priority i = PRIORITY_COUNT; i <= priority; i++) {
			if(shared_list_init(&(ARRAY_LIST_READY[i]))) {

				// Si una de las inicializaciones falla, Se trunca el array para sólo incluir las listas de READY que se pudieron inicializar
				new_array_list_ready = realloc(ARRAY_LIST_READY, sizeof(t_Shared_List) * i);
				if(new_array_list_ready == NULL) {
					log_error_r(&MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(t_Shared_List) * PRIORITY_COUNT, sizeof(t_Shared_List) * i);
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
	if((status = pthread_rwlock_unlock(&RWLOCK_ARRAY_READY))) {
		report_error_pthread_rwlock_unlock(status);
		return -1;
	}

	return 0;

}

int array_list_ready_destroy(void) {
	int retval = 0, status;
	for(t_Priority i = 0; i < PRIORITY_COUNT; i++) {
		if((status = pthread_mutex_destroy(&(ARRAY_LIST_READY[PRIORITY_COUNT - 1 - i].mutex)))) {
			report_error_pthread_mutex_destroy(status);
			retval = -1;
		}
		list_destroy_and_free_elements(ARRAY_LIST_READY[PRIORITY_COUNT - 1 - i].list);
	}
	free(ARRAY_LIST_READY);
	PRIORITY_COUNT = 0;
	return retval;
}

void log_state_list(t_Logger logger, const char *state_name, t_list *pcb_list) {
	char *pid_string = string_new();
	pcb_list_to_pid_string(pcb_list, &pid_string);
	log_trace_r(&logger, "%s: [%s]", state_name, pid_string);
	free(pid_string);
}

void pcb_list_to_pid_string(t_list *pcb_list, char **destination) {
	if(pcb_list == NULL || destination == NULL || *destination == NULL)
		return;

	t_link_element *element = pcb_list->head;

	if((**destination) && (element != NULL))
		string_append(destination, ", ");

	char *pid_as_string;
	while(element != NULL) {
        pid_as_string = string_from_format("%u", ((t_PCB *) element->data)->PID);
        string_append(destination, pid_as_string);
        free(pid_as_string);
		element = element->next;
        if(element != NULL)
            string_append(destination, ", ");
    }
}

void tcb_list_to_pid_tid_string(t_list *tcb_list, char **destination) {
	if(tcb_list == NULL || destination == NULL || *destination == NULL)
		return;

	t_link_element *element = tcb_list->head;

	if((**destination) && (element != NULL))
		string_append(destination, ", ");

	char *pid_tid_as_string;
	while(element != NULL) {
        pid_tid_as_string = string_from_format("%u:%u", ((t_TCB *) element->data)->pcb->PID, ((t_TCB *) element->data)->TID);
        string_append(destination, pid_tid_as_string);
        free(pid_tid_as_string);
		element = element->next;
        if(element != NULL)
            string_append(destination, ", ");
    }
}

void dump_memory_list_to_pid_tid_string(t_list *dump_memory_list, char **destination) {
	if(dump_memory_list == NULL || destination == NULL || *destination == NULL)
		return;

	t_link_element *element = dump_memory_list->head;

	if((**destination) && (element != NULL))
		string_append(destination, ", ");

	char *pid_tid_as_string;
	while(element != NULL) {
        pid_tid_as_string = string_from_format("%u:%u", ((t_Dump_Memory_Petition *) element->data)->tcb->pcb->PID, ((t_Dump_Memory_Petition *) element->data)->tcb->TID);
        string_append(destination, pid_tid_as_string);
        free(pid_tid_as_string);
		element = element->next;
        if(element != NULL)
            string_append(destination, ", ");
    }
}

int wait_dump_memory_threads(void) {
	
    int retval = 0, status;

	if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
		report_error_pthread_rwlock_rdlock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

		if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex)))) {
			report_error_pthread_mutex_lock(status);
			retval = -1;
			goto cleanup_rwlock_scheduling;
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex));

			t_link_element *current = SHARED_LIST_BLOCKED_DUMP_MEMORY.list->head;
			while(current != NULL) {
				t_Dump_Memory_Petition *dump_memory_petition = current->data;
				if((status = pthread_cancel(dump_memory_petition->bool_thread.thread))) {
					report_error_pthread_cancel(status);
					retval = -1;
					goto cleanup_mutex_clients;
				}
				current = current->next;
			}

			//log_trace_r(&MODULE_LOGGER, "Esperando a que finalicen los hilos de petición de volcado de memoria");
			
			while(SHARED_LIST_BLOCKED_DUMP_MEMORY.list->head != NULL) {
				if((status = pthread_cond_wait(&COND_BLOCKED_DUMP_MEMORY, &(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex)))) {
					report_error_pthread_cond_wait(status);
					retval = -1;
					break;
				}
			}

		cleanup_mutex_clients:
		pthread_cleanup_pop(0); // SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex
		if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex)))) {
			report_error_pthread_mutex_unlock(status);
			retval = -1;
			goto cleanup_rwlock_scheduling;
		}

	cleanup_rwlock_scheduling:
	pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
	if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
		report_error_pthread_rwlock_unlock(status);
		retval = -1;
		goto ret;
	}

	ret:
    return retval;
}