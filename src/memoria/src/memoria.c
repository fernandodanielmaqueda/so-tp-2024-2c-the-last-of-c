
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
    MODULE_LOG_PATHNAME = "memoria.log";
    MODULE_CONFIG_PATHNAME = "memoria.config";


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


    pthread_t thread_main = pthread_self();


	// Crea hilo para manejar señales
	if((status = pthread_create(&THREAD_SIGNAL_MANAGER, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main))) {
		log_error_pthread_create(status);
		return EXIT_FAILURE;
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_SIGNAL_MANAGER);


    // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_init(&RWLOCK_PARTITIONS_AND_PROCESSES, NULL))) {
        log_error_pthread_rwlock_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &RWLOCK_PARTITIONS_AND_PROCESSES);

    // PARTITION_TABLE
    PARTITION_TABLE = list_create();
    if(PARTITION_TABLE == NULL) {
        log_error(MODULE_LOGGER, "list_create: No se pudo crear la tabla de particiones");
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
	if((status = pthread_mutex_init(&MUTEX_MINIMAL_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_MINIMAL_LOGGER);
	if(initialize_logger(&MINIMAL_LOGGER, MINIMAL_LOG_PATHNAME, "Minimal")) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MINIMAL_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_MODULE_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_MODULE_LOGGER);
	if(initialize_logger(&MODULE_LOGGER, MODULE_LOG_PATHNAME, MODULE_NAME)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MODULE_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_SOCKET_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_SOCKET_LOGGER);
	if(initialize_logger(&SOCKET_LOGGER, SOCKET_LOG_PATHNAME, "Socket")) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SOCKET_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_SERIALIZE_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_SERIALIZE_LOGGER);
	if(initialize_logger(&SERIALIZE_LOGGER, SERIALIZE_LOG_PATHNAME, "Serialize")) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SERIALIZE_LOGGER);


    // MAIN_MEMORY
    MAIN_MEMORY = malloc(MEMORY_SIZE);
    if(MAIN_MEMORY == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para la memoria principal.", MEMORY_SIZE);
        exit_sigint();
    }
	pthread_cleanup_push((void (*)(void *)) free, (void *) MAIN_MEMORY);

    memset(MAIN_MEMORY, 0, MEMORY_SIZE);


	// COND_CLIENTS
	if((status = pthread_cond_init(&COND_CLIENTS, NULL))) {
		log_error_pthread_cond_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_CLIENTS);

    // MUTEX_JOBS_KERNEL
    if((status = pthread_mutex_init(&(SHARED_LIST_CLIENTS.mutex), NULL))) {
        log_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_CLIENTS.mutex));

    // LIST_JOBS_KERNEL
    SHARED_LIST_CLIENTS.list = list_create();
    if(SHARED_LIST_CLIENTS.list == NULL) {
        log_error(MODULE_LOGGER, "list_create: No se pudo crear la lista de clientes del kernel");
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) list_destroy, SHARED_LIST_CLIENTS.list);


    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente", MODULE_NAME);


	// Sockets
    if((status = pthread_mutex_init(&MUTEX_CLIENT_CPU, NULL))) {
        log_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_CLIENT_CPU);


	// [Servidor] Memoria <- [Cliente(s)] Kernel + CPU
    // TODO: pthread_cleanup_push con pthread_cond_wait hasta que terminen todos los clientes
    server_thread_coordinator(&SERVER_MEMORY, memory_client_handler);


	// Cleanup
    pthread_cleanup_pop(1); // MUTEX_CLIENT_CPU
	pthread_cleanup_pop(1); // LIST_JOBS_KERNEL
	pthread_cleanup_pop(1); // MUTEX_JOBS_KERNEL
	pthread_cleanup_pop(1); // COND_CLIENTS
	pthread_cleanup_pop(1); // MAIN_MEMORY
	pthread_cleanup_pop(1); // SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // SOCKET_LOGGER
	pthread_cleanup_pop(1); // MUTEX_SOCKET_LOGGER
	pthread_cleanup_pop(1); // MODULE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_MODULE_LOGGER
	pthread_cleanup_pop(1); // MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MUTEX_MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MODULE_CONFIG
	pthread_cleanup_pop(1); // PARTITION_TABLE
	pthread_cleanup_pop(1); // RWLOCK_PARTITIONS_AND_PROCESSES
	pthread_cleanup_pop(1); // THREAD_SIGNAL_MANAGER

    return EXIT_SUCCESS;
}
/*
t_Partition *partition_create() {

}
*/

int partition_destroy(t_Partition *partition) {
    int retval = 0, status;

    if((status = pthread_rwlock_destroy(&(partition->rwlock_partition)))) {
        log_error_pthread_rwlock_destroy(status);
        retval = -1;
    }

    free(partition);

    return retval;
}

int partition_table_destroy(void) {
    int retval = 0, status;

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
    int retval = 0, status;

    if(!config_has_properties(MODULE_CONFIG, "PUERTO_ESCUCHA", "IP_FILESYSTEM", "PUERTO_FILESYSTEM", "TAM_MEMORIA", "PATH_INSTRUCCIONES", "RETARDO_RESPUESTA", "ESQUEMA", "ALGORITMO_BUSQUEDA", "PARTICIONES", "LOG_LEVEL", NULL)) {
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
                size_t base = 0;
                t_Partition *new_partition;
                for(register unsigned int i = 0; fixed_partitions[i] != NULL; i++) {
                    new_partition = malloc(sizeof(t_Partition));
                    if(new_partition == NULL) {
                        fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
                        retval = -1;
                        goto cleanup_fixed_partitions;
                    }
                    pthread_cleanup_push((void (*)(void *)) free, new_partition);

                        new_partition->size = strtoul(fixed_partitions[i], &end, 10);
                        if(!*(fixed_partitions[i]) || *end || new_partition->size == 0) {
                            fprintf(stderr, "%s: valor de la clave PARTICIONES invalido: el tamaño de la partición %u no es un número entero válido: %s\n", MODULE_CONFIG_PATHNAME, i, fixed_partitions[i]);
                            retval = -1;
                            goto cleanup_new_partition;
                        }

                        if((status = pthread_rwlock_init(&(new_partition->rwlock_partition), NULL))) {
                            log_error_pthread_rwlock_init(status);
                            retval = -1;
                            goto cleanup_new_partition;
                        }
                        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &(new_partition->rwlock_partition));

                        new_partition->base = base;
                        new_partition->occupied = false;

                        list_add(PARTITION_TABLE, new_partition);

                        base += new_partition->size;

                        pthread_cleanup_pop(0); // rwlock
                        if(retval) {
                            if((status = pthread_rwlock_destroy(&(new_partition->rwlock_partition)))) {
                                log_error_pthread_rwlock_destroy(status);
                            }
                        }

                    cleanup_new_partition:
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
            t_Partition *new_partition = malloc(sizeof(t_Partition));
            if(new_partition == NULL) {
                fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
                return -1;
            }

            new_partition->base = 0;
            new_partition->size = MEMORY_SIZE;
            new_partition->occupied = false;

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

void free_threads(t_Memory_Process *process) {

    for(t_PID pid = 0; pid < process->tid_count; pid++) {
        //Free instrucciones
        if(process->array_memory_threads[pid] != NULL) {
            for(t_PC pc = 0; pc < process->array_memory_threads[pid]->instructions_count; pc++) {
                free(process->array_memory_threads[pid]->array_instructions[pc]);
            }
            free(process->array_memory_threads[pid]->array_instructions);
            free(process->array_memory_threads[pid]);
        }
    }

    free(process->array_memory_threads);

}

void free_memory(void) {
    //int status;

    // Free particiones
    for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
        t_Partition* partition = list_get(PARTITION_TABLE, i);
        free(partition);
    }

    // Free procesos
    for(t_PID pid = 0; pid < PID_COUNT; pid++) {
        if(ARRAY_PROCESS_MEMORY[pid] != NULL) {
            process_destroy(ARRAY_PROCESS_MEMORY[pid]);
        }

        ARRAY_PROCESS_MEMORY[pid] = NULL;
    }

    free(ARRAY_PROCESS_MEMORY);
    free(MAIN_MEMORY);

}