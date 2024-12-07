
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "memoria.h"

size_t MEMORY_SIZE;
char *INSTRUCTIONS_PATH;
int RESPONSE_DELAY;

t_Memory_Management_Scheme MEMORY_MANAGEMENT_SCHEMES[] = {
    [FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME] = {.name = "FIJAS", .function = NULL },
    [DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME] = {.name = "DINAMICAS", .function = NULL }
};

e_Memory_Management_Scheme MEMORY_MANAGEMENT_SCHEME;

t_Memory_Allocation_Algorithm MEMORY_ALLOCATION_ALGORITHMS[] = {
    [FIRST_FIT_MEMORY_ALLOCATION_ALGORITHM] = {.name = "FIRST", .function = NULL },
    [BEST_FIT_MEMORY_ALLOCATION_ALGORITHM] = {.name = "BEST", .function = NULL },
    [WORST_FIT_MEMORY_ALLOCATION_ALGORITHM] = {.name = "WORST", .function = NULL }
};

e_Memory_Allocation_Algorithm MEMORY_ALLOCATION_ALGORITHM;

void *MAIN_MEMORY;

pthread_rwlock_t RWLOCK_PARTITIONS_AND_PROCESSES;

t_list *PARTITION_TABLE;

t_PID PID_COUNT = 0;
t_Memory_Process **ARRAY_PROCESS_MEMORY = NULL;

int module(int argc, char* argv[]) {
    int status;

    MODULE_NAME = "memoria";
    MODULE_CONFIG_PATHNAME = "memoria.config";


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


    pthread_t thread_main = pthread_self();


	// Crea hilo para manejar señales
	if((status = pthread_create(&THREAD_SIGNAL_MANAGER, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main))) {
		report_error_pthread_create(status);
		return EXIT_FAILURE;
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_SIGNAL_MANAGER);


    // PARTITION_TABLE
    PARTITION_TABLE = list_create();
    if(PARTITION_TABLE == NULL) {
        fprintf(stderr, "list_create: No se pudo crear la tabla de particiones\n");
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) partition_table_destroy, NULL);


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


    // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_init(&RWLOCK_PARTITIONS_AND_PROCESSES, NULL))) {
        report_error_pthread_rwlock_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &RWLOCK_PARTITIONS_AND_PROCESSES);

    // ARRAY_PROCESS_MEMORY
    pthread_cleanup_push((void (*)(void *)) array_memory_processes_destroy, (void *) ARRAY_PROCESS_MEMORY);


    // MAIN_MEMORY
    MAIN_MEMORY = malloc(MEMORY_SIZE);
    if(MAIN_MEMORY == NULL) {
        log_error_r(&MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para la memoria principal.", MEMORY_SIZE);
        exit_sigint();
    }
	pthread_cleanup_push((void (*)(void *)) free, (void *) MAIN_MEMORY);

    memset(MAIN_MEMORY, 0, MEMORY_SIZE);


	// COND_CLIENTS
	if((status = pthread_cond_init(&COND_CLIENTS, NULL))) {
		report_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_CLIENTS);

    // LIST_MEMORY_JOBS
    if((status = pthread_mutex_init(&(SHARED_LIST_CLIENTS.mutex), NULL))) {
        report_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_CLIENTS.mutex));

    // LIST_MEMORY_JOBS
    SHARED_LIST_CLIENTS.list = list_create();
    if(SHARED_LIST_CLIENTS.list == NULL) {
        log_error_r(&MODULE_LOGGER, "list_create: No se pudo crear la lista de clientes del kernel");
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) list_destroy, SHARED_LIST_CLIENTS.list);


    // Sockets
    if((status = pthread_mutex_init(&MUTEX_CLIENT_CPU, NULL))) {
        report_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_CLIENT_CPU);

    log_debug_r(&MODULE_LOGGER, "Modulo %s inicializado correctamente", MODULE_NAME);

	// [Servidor] Memoria <- [Cliente(s)] Kernel + CPU
    pthread_cleanup_push((void (*)(void *)) wait_client_threads, NULL);
    server_thread_coordinator(&SERVER_MEMORY, memory_client_handler);


	// Cleanup
    pthread_cleanup_pop(1); // wait_client_threads
    pthread_cleanup_pop(1); // MUTEX_CLIENT_CPU
	pthread_cleanup_pop(1); // LIST_MEMORY_JOBS
	pthread_cleanup_pop(1); // MUTEX_MEMORY_JOBS
	pthread_cleanup_pop(1); // COND_CLIENTS
	pthread_cleanup_pop(1); // MAIN_MEMORY
	pthread_cleanup_pop(1); // ARRAY_PROCESS_MEMORY
	pthread_cleanup_pop(1); // RWLOCK_PARTITIONS_AND_PROCESSES
	pthread_cleanup_pop(1); // SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // SOCKET_LOGGER
	pthread_cleanup_pop(1); // MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MODULE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_LOGGERS
	pthread_cleanup_pop(1); // MODULE_CONFIG
	pthread_cleanup_pop(1); // PARTITION_TABLE
	pthread_cleanup_pop(1); // THREAD_SIGNAL_MANAGER

    return EXIT_SUCCESS;
}

int array_memory_processes_destroy(void) {
    int retval = 0;

    for(t_PID pid = 0; pid < PID_COUNT; pid++) {
        if(memory_process_destroy(ARRAY_PROCESS_MEMORY[pid])) {
            retval = -1;
        }

        ARRAY_PROCESS_MEMORY[pid] = NULL;
    }

    free(ARRAY_PROCESS_MEMORY);

    return retval;
}

int array_memory_threads_destroy(t_Memory_Process *process) {
    int retval = 0;

    for(t_TID tid = 0; tid < process->tid_count; tid++) {
        if(memory_thread_destroy(process->array_memory_threads[tid])) {
            retval = -1;
        }

    }

    free(process->array_memory_threads);

    return retval;
}

t_Memory_Process *memory_process_create(t_PID pid, size_t size) {
    int retval = 0, status;

    t_Memory_Process *new_process = malloc(sizeof(t_Memory_Process));
    if(new_process == NULL) {
        log_error_r(&MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo proceso.", sizeof(t_Memory_Process));
        return NULL;
    }
    pthread_cleanup_push((void (*)(void *)) free, new_process);

    new_process->pid = pid;
    new_process->size = size;
    new_process->tid_count = 0;
    new_process->array_memory_threads = NULL;

    if((status = pthread_rwlock_init(&(new_process->rwlock_array_memory_threads), NULL))) {
        report_error_pthread_rwlock_init(status);
        retval = -1;
        goto cleanup_new_process;
    }

    cleanup_new_process:
    pthread_cleanup_pop(retval); // new_process

    if(retval) {
        return NULL;
    }

    return new_process;
}

int memory_process_destroy(t_Memory_Process *process) {
    int retval = 0, status;

    if(process == NULL) {
        return 0;
    }

    if(array_memory_threads_destroy(process)) {
        retval = -1;
    }

    if((status = pthread_rwlock_destroy(&(process->rwlock_array_memory_threads)))) {
        report_error_pthread_rwlock_destroy(status);
        retval = -1;
    }

    free(process);

    return retval;
}

t_Memory_Thread *memory_thread_create(t_TID tid, char *argument_path) {
    int retval = 0;

    t_Memory_Thread *new_thread = malloc(sizeof(t_Memory_Thread));
    if(new_thread == NULL) {
        log_error_r(&MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el hilo", sizeof(t_Memory_Thread));
        return NULL;
    }
    pthread_cleanup_push((void (*)(void *)) free, new_thread);

    new_thread->tid = tid;
    new_thread->instructions_count = 0;
    new_thread->array_instructions = NULL;

    //Inicializar registros
    new_thread->registers.PC = 0;
    new_thread->registers.AX = 0;
    new_thread->registers.BX = 0;
    new_thread->registers.CX = 0;
    new_thread->registers.DX = 0;
    new_thread->registers.EX = 0;
    new_thread->registers.FX = 0;
    new_thread->registers.GX = 0;
    new_thread->registers.HX = 0;

    pthread_cleanup_pop(retval); // new_thread

    if(retval) {
        return NULL;
    }

    return new_thread;

}

int memory_thread_destroy(t_Memory_Thread *thread) {
    if(thread == NULL) {
        return 0;
    }

    for(t_PC pc = 0; pc < thread->instructions_count; pc++) {
        free(thread->array_instructions[pc]);
    }
    free(thread->array_instructions);

    free(thread);

    return 0;
}

t_Partition *partition_create(size_t size, size_t base) {
    int retval = 0, status;

    t_Partition *new_partition = malloc(sizeof(t_Partition));
    if(new_partition == NULL) {
        fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
        return NULL;
    }
    pthread_cleanup_push((void (*)(void *)) free, new_partition);

        if((status = pthread_rwlock_init(&(new_partition->rwlock_partition), NULL))) {
            report_error_pthread_rwlock_init(status);
            retval = -1;
            goto cleanup_new_partition;
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &(new_partition->rwlock_partition));

            new_partition->size = size;
            new_partition->base = base;
            new_partition->occupied = false;

            pthread_cleanup_pop(0); // rwlock
            if(retval) {
                if((status = pthread_rwlock_destroy(&(new_partition->rwlock_partition)))) {
                    report_error_pthread_rwlock_destroy(status);
                }
            }

    cleanup_new_partition:
    pthread_cleanup_pop(retval);

    if(retval) {
        return NULL;
    }
    else {
        return new_partition;
    }
}


int partition_destroy(t_Partition *partition) {
    int retval = 0, status;

    if((status = pthread_rwlock_destroy(&(partition->rwlock_partition)))) {
        report_error_pthread_rwlock_destroy(status);
        retval = -1;
    }

    free(partition);

    return retval;
}

int partition_table_destroy(void) {
    int retval = 0;

    while(list_size(PARTITION_TABLE) > 0) {
        t_Partition *partition = list_remove(PARTITION_TABLE, 0);
        if(partition_destroy(partition)) {
            retval = -1;
        }
    }

    list_destroy(PARTITION_TABLE);

    return retval;
}

int read_module_config(t_config *MODULE_CONFIG) {
    int retval = 0;

    if(!config_has_properties(MODULE_CONFIG, "PUERTO_ESCUCHA", "IP_FILESYSTEM", "PUERTO_FILESYSTEM", "TAM_MEMORIA", "PATH_INSTRUCCIONES", "RETARDO_RESPUESTA", "ESQUEMA", "ALGORITMO_BUSQUEDA", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

    char *string;

    string = config_get_string_value(MODULE_CONFIG, "ESQUEMA");
	if(memory_management_scheme_find(string, &MEMORY_MANAGEMENT_SCHEME)) {
		fprintf(stderr, "%s: valor de la clave ESQUEMA invalido: %s\n", MODULE_CONFIG_PATHNAME, string);
		return -1;
	}

    string = config_get_string_value(MODULE_CONFIG, "ALGORITMO_BUSQUEDA");
	if(memory_allocation_algorithm_find(string, &MEMORY_ALLOCATION_ALGORITHM)) {
		fprintf(stderr, "%s: valor de la clave ALGORITMO_BUSQUEDA invalido: %s\n", MODULE_CONFIG_PATHNAME, string);
		return -1;
	}

    MEMORY_SIZE = (size_t) config_get_int_value(MODULE_CONFIG, "TAM_MEMORIA");

    // Creación inicial de particiones
    switch(MEMORY_MANAGEMENT_SCHEME) {

        case FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        {
            char **fixed_partitions = config_get_array_value(MODULE_CONFIG, "PARTICIONES");
            if(fixed_partitions == NULL) {
                fprintf(stderr, "%s: la clave PARTICIONES no tiene valor\n", MODULE_CONFIG_PATHNAME);
                return -1;
            }
            pthread_cleanup_push((void (*)(void *)) string_array_destroy, fixed_partitions);

                char *end;
                size_t size, base = 0;
                t_Partition *new_partition;
                for(register unsigned int i = 0; fixed_partitions[i] != NULL; i++) {

                    size = strtoul(fixed_partitions[i], &end, 10);
                    if((!(*(fixed_partitions[i]))) || (*end) || size == 0) {
                        fprintf(stderr, "%s: valor de la clave PARTICIONES invalido: el tamaño de la partición %u no es un número entero válido: %s\n", MODULE_CONFIG_PATHNAME, i, fixed_partitions[i]);
                        retval = -1;
                        goto cleanup_fixed_partitions;
                    }

                    new_partition = partition_create(size, base);
                    if(new_partition == NULL) {
                        retval = -1;
                        goto cleanup_fixed_partitions;
                    }
                    pthread_cleanup_push((void (*)(void *)) partition_destroy, new_partition);

                        list_add(PARTITION_TABLE, new_partition);

                        base += new_partition->size;

                    pthread_cleanup_pop(retval); // new_partition

                    if(retval) {
                        goto cleanup_fixed_partitions;
                    }
                }

                if(list_size(PARTITION_TABLE) == 0) {
                    fprintf(stderr, "%s: valor de la clave PARTICIONES invalido\n", MODULE_CONFIG_PATHNAME);
                    retval = -1;
                    goto cleanup_fixed_partitions;
                }

            cleanup_fixed_partitions:
            pthread_cleanup_pop(1); // fixed_partitions
            break;
        }

        case DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        {
            t_Partition *new_partition = partition_create(MEMORY_SIZE, 0);
            if(new_partition == NULL) {
                return -1;
            }

            list_add(PARTITION_TABLE, new_partition);

            break;
        }

    }

    if(retval) {
        return -1;
    }

    SERVER_MEMORY = (t_Server) {.server_type = MEMORY_PORT_TYPE, .clients_type = TO_BE_IDENTIFIED_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA")};

    INSTRUCTIONS_PATH = config_get_string_value(MODULE_CONFIG, "PATH_INSTRUCCIONES");
        if(INSTRUCTIONS_PATH[0]) {

            size_t length = strlen(INSTRUCTIONS_PATH);
            if(INSTRUCTIONS_PATH[length - 1] == '/') {
                INSTRUCTIONS_PATH[length - 1] = '\0';
            }

            DIR *dir = opendir(INSTRUCTIONS_PATH);
            if(dir == NULL) {
                fprintf(stderr, "%s: No se pudo abrir el directorio indicado en el valor de PATH_INSTRUCCIONES: %s\n", MODULE_CONFIG_PATHNAME, INSTRUCTIONS_PATH);
                return -1;
            }
            closedir(dir);
        }

    RESPONSE_DELAY = config_get_int_value(MODULE_CONFIG, "RETARDO_RESPUESTA");

    LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));

    return 0;
}

int memory_management_scheme_find(char *name, e_Memory_Management_Scheme *destination) {
    if(name == NULL || destination == NULL)
        return -1;
    
    size_t memory_management_schemes_number = sizeof(MEMORY_MANAGEMENT_SCHEMES) / sizeof(MEMORY_MANAGEMENT_SCHEMES[0]);
    for(register e_Memory_Management_Scheme memory_management_scheme = 0; memory_management_scheme < memory_management_schemes_number; memory_management_scheme++)
        if(strcmp(MEMORY_MANAGEMENT_SCHEMES[memory_management_scheme].name, name) == 0) {
            *destination = memory_management_scheme;
            return 0;
        }

    return -1;
}

int memory_allocation_algorithm_find(char *name, e_Memory_Allocation_Algorithm *destination) {
    if(name == NULL || destination == NULL)
        return -1;
    
    size_t memory_allocation_algorithms_number = sizeof(MEMORY_ALLOCATION_ALGORITHMS) / sizeof(MEMORY_ALLOCATION_ALGORITHMS[0]);
    for(register e_Memory_Allocation_Algorithm memory_allocation_algorithm = 0; memory_allocation_algorithm < memory_allocation_algorithms_number; memory_allocation_algorithm++)
        if(strcmp(MEMORY_ALLOCATION_ALGORITHMS[memory_allocation_algorithm].name, name) == 0) {
            *destination = memory_allocation_algorithm;
            return 0;
        }

    return -1;
}