
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
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_SIGNAL_MANAGER);


    // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_init(&RWLOCK_PARTITIONS_AND_PROCESSES, NULL))) {
        log_error_pthread_rwlock_init(status);
        pthread_exit(NULL);
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &RWLOCK_PARTITIONS_AND_PROCESSES);

    // PARTITION_TABLE
    PARTITION_TABLE = list_create();
    if(PARTITION_TABLE == NULL) {
        log_error(MODULE_LOGGER, "list_create: No se pudo crear la tabla de particiones");
        pthread_exit(NULL);
    }
    pthread_cleanup_push((void (*)(void *)) list_destroy, PARTITION_TABLE);
    //  TODO: pthread_cleanup_push((void (*)(void *)) , PARTITION_TABLE);


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
	if((status = pthread_mutex_init(&MUTEX_MINIMAL_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_MINIMAL_LOGGER);
	if(initialize_logger(&MINIMAL_LOGGER, MINIMAL_LOG_PATHNAME, "Minimal")) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MINIMAL_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_MODULE_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_MODULE_LOGGER);
	if(initialize_logger(&MODULE_LOGGER, MODULE_LOG_PATHNAME, MODULE_NAME)) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MODULE_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_SOCKET_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_SOCKET_LOGGER);
	if(initialize_logger(&SOCKET_LOGGER, SOCKET_LOG_PATHNAME, "Socket")) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SOCKET_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_SERIALIZE_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_SERIALIZE_LOGGER);
	if(initialize_logger(&SERIALIZE_LOGGER, SERIALIZE_LOG_PATHNAME, "Serialize")) {
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SERIALIZE_LOGGER);


    // MAIN_MEMORY
    MAIN_MEMORY = malloc(MEMORY_SIZE);
    if(MAIN_MEMORY == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para la memoria principal.", MEMORY_SIZE);
        pthread_exit(NULL);
    }
	pthread_cleanup_push((void (*)(void *)) free, (void *) MAIN_MEMORY);

    memset(MAIN_MEMORY, 0, MEMORY_SIZE);


	// COND_JOBS_KERNEL
	if((status = pthread_cond_init(&COND_JOBS_KERNEL, NULL))) {
		log_error_pthread_cond_init(status);
		pthread_exit(NULL);
	}
	pthread_cleanup_push((void (*)(void *)) pthread_cond_destroy, (void *) &COND_JOBS_KERNEL);

    // MUTEX_JOBS_KERNEL
    if((status = pthread_mutex_init(&(SHARED_LIST_JOBS_KERNEL.mutex), NULL))) {
        log_error_pthread_mutex_init(status);
        pthread_exit(NULL);
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &(SHARED_LIST_JOBS_KERNEL.mutex));

    // LIST_JOBS_KERNEL
    SHARED_LIST_JOBS_KERNEL.list = list_create();
    if(SHARED_LIST_JOBS_KERNEL.list == NULL) {
        log_error(MODULE_LOGGER, "list_create: No se pudo crear la lista de clientes del kernel");
        pthread_exit(NULL);
    }
    pthread_cleanup_push((void (*)(void *)) list_destroy, SHARED_LIST_JOBS_KERNEL.list);


    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente", MODULE_NAME);


	// Sockets
	pthread_cleanup_push((void (*)(void *)) finish_sockets, NULL);
	initialize_sockets();


	// Cleanup

	pthread_cleanup_pop(1); // Sockets
	pthread_cleanup_pop(1); // LIST_JOBS_KERNEL
	pthread_cleanup_pop(1); // MUTEX_JOBS_KERNEL
	pthread_cleanup_pop(1); // COND_JOBS_KERNEL
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

int read_module_config(t_config *MODULE_CONFIG) {
    int retval = 0;

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
                        if(!*(fixed_partitions[i]) || *end) {
                            fprintf(stderr, "%s: valor de la clave PARTICIONES invalido: el tamaño de la partición %u no es un número entero válido: %s\n", MODULE_CONFIG_PATHNAME, i, fixed_partitions[i]);
                            retval = -1;
                            goto cleanup_new_partition;
                        }

                        new_partition->base = base;
                        new_partition->occupied = false;

                        list_add(PARTITION_TABLE, new_partition);

                        base += new_partition->size;

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

void listen_kernel(int fd_client) {

    t_Package* package = package_create();
    if(package == NULL) {
        // TODO
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) package_destroy, package);

    if(package_receive(package, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al recibir paquete de [Cliente] %s", fd_client, PORT_NAMES[KERNEL_PORT_TYPE]);
        error_pthread();
    }
    log_trace(MODULE_LOGGER, "[%d] Se recibe paquete de [Cliente] %s", fd_client, PORT_NAMES[KERNEL_PORT_TYPE]);

    switch(package->header) {

        case PROCESS_CREATE_HEADER:
            attend_process_create(fd_client, &(package->payload));
            break;

        case PROCESS_DESTROY_HEADER:
            attend_process_destroy(fd_client, &(package->payload));
            break;
        
        case THREAD_CREATE_HEADER:
            attend_thread_create(fd_client, &(package->payload));
            break;
            
        case THREAD_DESTROY_HEADER:
            attend_thread_destroy(fd_client, &(package->payload));
            //send_result_with_header(THREAD_DESTROY_HEADER, ((retval || (partition == NULL)) ? 1 : 0), fd_client);
            break;
            
        case MEMORY_DUMP_HEADER:
            attend_memory_dump(fd_client, &(package->payload));
            //send_result_with_header(MEMORY_DUMP_HEADER, ((retval || (partition == NULL)) ? 1 : 0), fd_client);
            break;

        default:
            log_warning(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
            break;

    }

    pthread_cleanup_pop(1); // package_destroy
}

void attend_process_create(int fd_client, t_Payload *payload) {

    int retval = 0, status;

    t_PID pid;
    size_t size;

    payload_remove(payload, &pid, sizeof(pid));
    size_deserialize(payload, &size);

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de creación de proceso de [Cliente] %s [PID: %u - Tamaño: %zu]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, size);

    // TODO: ¿Y si el proceso ya existe? ¿Lo piso? ¿Lo ignoro? ¿Termino el programa?

    t_Partition *partition = NULL;

    t_Memory_Process *new_process = malloc(sizeof(t_Memory_Process));
    if(new_process == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo proceso.", sizeof(t_Memory_Process));
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) free, new_process);

    new_process->pid = pid;
    new_process->size = size;
    new_process->tid_count = 0;
    new_process->array_memory_threads = NULL;

    if((status = pthread_rwlock_init(&(new_process->rwlock_array_memory_threads), NULL))) {
        log_error_pthread_rwlock_init(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, &(new_process->rwlock_array_memory_threads));

    if((status = pthread_rwlock_wrlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_wrlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

    // Asignar partición
    allocate_partition(&partition, new_process->size);

    if(partition == NULL) {
        log_warning(MODULE_LOGGER, "[%d] No hay particiones disponibles para la solicitud de creación de proceso %u", fd_client, new_process->pid);
        retval = -1;
        goto cleanup_rwlock_proceses_and_partitions;
    }

    log_debug(MODULE_LOGGER, "[%d] Particion asignada para el pedido del proceso %u", fd_client, new_process->pid);
    partition->occupied = true;
    partition->pid = pid;
    new_process->partition = partition;

    if(add_element_to_array_process(new_process)) {
        log_debug(MODULE_LOGGER, "[%d] No se pudo agregar nuevo proceso al listado para el pedido del proceso %d", fd_client, new_process->pid);
        error_pthread();
    }

    log_info(MINIMAL_LOGGER, "## Proceso Creado - PID: %u - TAMAÑO: %zu", new_process->pid, new_process->size);

    cleanup_rwlock_proceses_and_partitions:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_unlock(status);
    }

    pthread_cleanup_pop(0); // rwlock_array_memory_threads
    if(partition == NULL) {
        if((status = pthread_rwlock_destroy(&(new_process->rwlock_array_memory_threads)))) {
            log_error_pthread_rwlock_destroy(status);
        }
    }

    pthread_cleanup_pop(retval); // new_process

    if(send_result_with_header(PROCESS_CREATE_HEADER, ((retval) ? 1 : 0), fd_client)) {
        // TODO
        error_pthread();
    }
}

void allocate_partition(t_Partition **partition, size_t required_size) {
    if(partition == NULL) {
        error_pthread();
    }

    int retval = 0;
    *partition = NULL;
    size_t index_partition;
    t_Partition *aux_partition;

    switch(MEMORY_ALLOCATION_ALGORITHM) {
        case FIRST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((!(aux_partition->occupied)) && ((aux_partition->size) >= required_size)) {
                    *partition = aux_partition;
                    index_partition = i;
                    break;
                }
            }

            break;
        }

        case BEST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((!(aux_partition->occupied)) && ((aux_partition->size) >= required_size)) {
                    if((*partition) == NULL) {
                        *partition = aux_partition;
                        index_partition = i;
                    }
                    else {
                        if((aux_partition->size) < ((*partition)->size)) {
                            *partition = aux_partition;
                            index_partition = i;
                        }
                    }
                }
            }

            break;
        }

        case WORST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((!(aux_partition->occupied)) && ((aux_partition->size) >= required_size)) {
                    if((*partition) == NULL) {
                        *partition = aux_partition;
                        index_partition = i;
                    }
                    else {
                        if((aux_partition->size) > ((*partition)->size)) {
                            *partition = aux_partition;
                            index_partition = i;
                        }
                    }
                }
            }

            break;
        }
    }

    if((*partition) != NULL) {
        // Realiza el fraccionamiento de la particion (si es requerido)
        switch(MEMORY_MANAGEMENT_SCHEME) {

            case FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME: {
                break;
            }

            case DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME: {

                if(split_partition(index_partition, required_size)) {
                    error_pthread();
                }

                break;
            }
        }
    }
}

int add_element_to_array_process(t_Memory_Process* process) {

    ARRAY_PROCESS_MEMORY = realloc(ARRAY_PROCESS_MEMORY, sizeof(t_Memory_Process *) * (PID_COUNT + 1));    
    if(ARRAY_PROCESS_MEMORY == NULL) {
        log_warning(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(t_Memory_Process *) * PID_COUNT, sizeof(t_Memory_Process *) * (PID_COUNT +1));
        return -1;
    }

    ARRAY_PROCESS_MEMORY[PID_COUNT] = process;
    PID_COUNT++;

    return 0;
}

int split_partition(size_t index_partition, size_t required_size) {

    t_Partition *old_partition = list_get(PARTITION_TABLE, index_partition);

    if(old_partition->size == required_size) {
        return 0;
    }

    t_Partition *new_partition = malloc(sizeof(t_Partition));
    if(new_partition == NULL) {
        fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
        exit(EXIT_FAILURE);
    }

    new_partition->size = (old_partition->size - required_size);
    new_partition->base = (old_partition->base + required_size);
    new_partition->occupied = false;

    old_partition->size = required_size;

    index_partition++;

    list_add_in_index(PARTITION_TABLE, index_partition, new_partition);

    return 0;

}

void attend_process_destroy(int fd_client, t_Payload *payload) {

    int status;

    t_PID pid;

    payload_remove(payload, &pid, sizeof(pid));

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de finalización de proceso de [Cliente] %s [PID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid);

    size_t size;
    t_Memory_Process *process = NULL;

    if((status = pthread_rwlock_wrlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_wrlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_error(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
            goto cleanup_rwlock_proceses_and_partitions;
        }

        process = ARRAY_PROCESS_MEMORY[pid];
        ARRAY_PROCESS_MEMORY[pid] = NULL;
        process->partition->occupied = false;
        size = process->size;

        if(MEMORY_MANAGEMENT_SCHEME == DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME)
            if(verify_and_join_splited_partitions(process->partition)) {
                error_pthread();
            }

    cleanup_rwlock_proceses_and_partitions:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }

    if(process_destroy(process)) {
        error_pthread();
    }

    log_info(MINIMAL_LOGGER, "## Proceso Destruido - PID: %u - TAMAÑO: %zu", pid, size);

    if(send_result_with_header(PROCESS_DESTROY_HEADER, ((process == NULL) ? 1 : 0), fd_client)) {
        // TODO
        error_pthread();
    }
}

int process_destroy(t_Memory_Process *process) {
    int retval = 0, status;

    // Liberacion de threads con sus struct
    free_threads(process);

    if((status = pthread_rwlock_destroy(&(process->rwlock_array_memory_threads)))) {
        log_error_pthread_rwlock_destroy(status);
        retval = -1;
    }

    free(process);

    return retval;
}

int verify_and_join_splited_partitions(t_Partition *partition) {

    t_link_element *current = PARTITION_TABLE->head;
    size_t i;
    for(i = 0; partition != (current->data); i++) {
        current = current->next;
    }

    // No es la primera ni la última partición
    if((i != 0) && (i != (list_size(PARTITION_TABLE) - 1))) {
        t_Partition *aux_partition_right;
        t_Partition *aux_partition_left;
    
        aux_partition_right = list_get(PARTITION_TABLE, (i + 1));
        aux_partition_left = list_get(PARTITION_TABLE, (i - 1));

        if(!(aux_partition_right->occupied)) {
            partition->size += aux_partition_right->size;
            list_remove_and_destroy_element(PARTITION_TABLE, (i + 1), free);
        }

        if(!(aux_partition_left->occupied)) {
            aux_partition_left->size += partition->size;
            list_remove_and_destroy_element(PARTITION_TABLE, i, free);
        }

        return 0;
    }

    // Es la primera partición (con cantidad de particiones > 1)
    if((i == 0) && (i != (list_size(PARTITION_TABLE) - 1))) {
        t_Partition *aux_partition_right;

        aux_partition_right = list_get(PARTITION_TABLE, (i + 1));

        if(!(aux_partition_right->occupied)) {
            partition->size += aux_partition_right->size;
            list_remove_and_destroy_element(PARTITION_TABLE, (i + 1), free);
        }

        return 0;
    }

    // Es la última partición (con cantidad de particiones > 1)
    if((i != 0) && (i == (list_size(PARTITION_TABLE) - 1))) {
        t_Partition *aux_partition_left;

        aux_partition_left = list_get(PARTITION_TABLE, (i - 1));

        if(!(aux_partition_left->occupied)) {
            aux_partition_left->size += partition->size;
            list_remove_and_destroy_element(PARTITION_TABLE, i, free);
        }

        return 0;
    }

    // Es la única partición
    return 0;
}

void attend_thread_create(int fd_client, t_Payload *payload) {
    int retval = 0, status;

    // TODO: ¿Y si el hilo ya existe? ¿Lo piso? ¿Lo ignoro? ¿Termino el programa?

    t_PID pid;
    t_TID tid;
    char *argument_path;

    payload_remove(payload, &pid, sizeof(pid));
    payload_remove(payload, &tid, sizeof(tid));
    text_deserialize(payload, &argument_path);

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de creación de hilo de [Cliente] %s [PID: %u - TID: %u - Archivo: %s]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid, argument_path);

    t_Memory_Thread *new_thread = malloc(sizeof(t_Memory_Thread));
    if(new_thread == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo thread.", sizeof(t_Memory_Thread));
        error_pthread();
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

    // Genera la ruta hacia el archivo de pseudocódigo
    char *target_path;
    // Ruta relativa
    target_path = malloc((INSTRUCTIONS_PATH[0] ? (strlen(INSTRUCTIONS_PATH) + 1) : 0) + strlen(argument_path) + 1);
    if(target_path == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudo reservar %zu bytes para la ruta relativa.", (INSTRUCTIONS_PATH[0] ? (strlen(INSTRUCTIONS_PATH) + 1) : 0) + strlen(argument_path) + 1);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) free, target_path);

    register int i;
    for(i = 0; INSTRUCTIONS_PATH[i]; i++) {
        target_path[i] = INSTRUCTIONS_PATH[i];
    }

    if(INSTRUCTIONS_PATH[0])
        target_path[i++] = '/';

    register int j;
    for(j = 0; argument_path[j]; j++) {
        target_path[i + j] = argument_path[j];
    }

    target_path[i + j] = '\0';

    log_debug(MODULE_LOGGER, "Ruta hacia el archivo de pseudocódigo: %s", target_path);

    // Inicializar instrucciones
    if(parse_pseudocode_file(target_path, &(new_thread->array_instructions), &(new_thread->instructions_count))) {
        // TODO
        error_pthread();
    }
    // TODO

    if((status = pthread_rwlock_rdlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_rdlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
            retval = -1;
            goto cleanup_rwlock_proceses_and_partitions;
        }

        if((status = pthread_rwlock_wrlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            log_error_pthread_rwlock_wrlock(status);
            error_pthread();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads));

            ARRAY_PROCESS_MEMORY[pid]->array_memory_threads = realloc(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads, sizeof(t_Memory_Thread *) * ((ARRAY_PROCESS_MEMORY[pid]->tid_count) + 1)); 
            if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads == NULL) {
                log_warning(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el array de threads.", sizeof(t_Memory_Thread *) * ((ARRAY_PROCESS_MEMORY[pid]->tid_count) + 1));
                error_pthread();
            }

            ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] = new_thread;
            (ARRAY_PROCESS_MEMORY[pid]->tid_count)++;

            log_info(MINIMAL_LOGGER, "## Hilo Creado - (PID:TID) - (%u:%u)", pid, tid);

        pthread_cleanup_pop(0); // rwlock_array_memory_threads
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            log_error_pthread_rwlock_unlock(status);
            error_pthread();
        }

    cleanup_rwlock_proceses_and_partitions:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }

    pthread_cleanup_pop(1); // target_path

    pthread_cleanup_pop(retval); // new_thread

    if(send_result_with_header(THREAD_CREATE_HEADER, ((retval) ? 1 : 0), fd_client)) {
        // TODO
        error_pthread();
    }
    // TODO

}

int attend_thread_destroy(int fd_client, t_Payload *payload) {

    t_PID pid;
    t_TID tid;

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de finalización de hilo de [Cliente] %s [PID: %u - TID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid);

    if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
        return -1;
    }

    if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el hilo %u:%u", pid, tid);
        return -1;
    }

    // Free instrucciones
    for(t_PC pc = 0; pc < ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count; pc++) {
        free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[pc]);
    }

    free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions);
    free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]);

    log_info(MINIMAL_LOGGER, "## Hilo Destruido - (PID:TID) - (%u:%u)", pid, tid);

    return 0;
}

int parse_pseudocode_file(char *path, char ***array_instruction, t_PC *count) {
    FILE* file;
    if((file = fopen(path, "r")) == NULL) {
        log_warning(MODULE_LOGGER, "%s: No se pudo abrir el archivo de pseudocodigo indicado.", path);
        return -1;
    }
    char *line = NULL, *subline;
    size_t length;
    ssize_t nread;
    while(1) {
        errno = 0;
        if((nread = getline(&line, &length, file)) == -1) {
            if(errno) {
                log_warning(MODULE_LOGGER, "getline: %s", strerror(errno));
                free(line);
                if(fclose(file)) {
                    log_error_fclose();
                }
                exit(EXIT_FAILURE);
            }
            // Se terminó de leer el archivo
            break;
        }
        // Ignora líneas en blanco
        subline = strip_whitespaces(line);
        if(*subline) { // Se leyó una línea con contenido
    
            *array_instruction = realloc(*array_instruction, (*count + 1) * sizeof(char *));
            if(*array_instruction == NULL) {
                perror("[Error] realloc memory for array fallo.");
                free(line);
                if(fclose(file)) {
                    log_error_fclose();
                }
                exit(EXIT_FAILURE);
            }
            (*array_instruction)[*count] = strdup(subline);
            if((*array_instruction)[*count] == NULL) {
                perror("[Error] malloc memory for string fallo.");
                free(line);
                if(fclose(file)) {
                    log_error_fclose();
                }
                exit(EXIT_FAILURE);
            }
            (*count)++;
        }
    }
    free(line);       
    if(fclose(file)) {
        log_error_fclose();
        return -1;
    }
    return 0;
}

void listen_cpu(void) {
    int result = 0, status;

    t_Package *package;

    while(1) {

        package = package_create();
        if(package == NULL) {
            // TODO
            return;
        }

        if(package_receive(package, CLIENT_CPU->fd_client)) {
            log_error(MODULE_LOGGER, "[%d] Error al recibir paquete de [Cliente] %s", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type]);
            exit(EXIT_FAILURE);
        }
        log_trace(MODULE_LOGGER, "[%d] Se recibe paquete de [Cliente] %s", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type]);

        switch(package->header) {
            case INSTRUCTION_REQUEST_HEADER:
                seek_instruccion(&(package->payload));
                package_destroy(package);
                break;

            case READ_REQUEST_HEADER:
                read_memory(&(package->payload));
                package_destroy(package);
                break;

            case WRITE_REQUEST_HEADER:
                write_memory(&(package->payload));
                package_destroy(package);
                break;

            case EXEC_CONTEXT_REQUEST_HEADER:
                seek_cpu_context(&(package->payload));
                package_destroy(package);
                break;

            case EXEC_CONTEXT_UPDATE_HEADER:
                update_cpu_context(&(package->payload));
                package_destroy(package);
                break;

            default:
                log_error(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
                package_destroy(package);
                break;
        }
    }
}

void seek_instruccion(t_Payload *payload) {

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;
    t_PC pc;

    payload_remove(payload, &pid, sizeof(pid));
    payload_remove(payload, &tid, sizeof(tid));
    payload_remove(payload, &pc, sizeof(pc));

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de instrucción de [Cliente] %s [PID: %u - TID: %u - PC: %u]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, pc);

    if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
        return;
    }

    if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el hilo %u:%u", pid, tid);
        return;
    }

    if(pc >= (ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count)) {
        log_debug(MODULE_LOGGER, "[ERROR] El ProgramCounter supera la cantidad de instrucciones para el hilo (PID:TID): (%u:%u)", pid, tid);
        return;
    }

    char *instruccionBuscada = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[pc];

    if(send_text_with_header(INSTRUCTION_REQUEST_HEADER, instruccionBuscada, CLIENT_CPU->fd_client)) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo enviar la instruccion del proceso %d", pid);
        return;
    }

    //FIX REQUIRED --> <> de instruccion
    log_info(MINIMAL_LOGGER, "## Obtener instruccion - (PID:TID) - (%u:%u) - Instruccion: %s", pid, tid, instruccionBuscada);

    return;
}

int read_memory(t_Payload *payload) {

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;
    size_t physical_address;
    size_t bytes;

    payload_remove(payload, &pid, sizeof(pid));
    payload_remove(payload, &tid, sizeof(tid));
    size_deserialize(payload, &physical_address);
    size_deserialize(payload, &bytes);

    log_info(MODULE_LOGGER, "[%d] Se recibe lectura en espacio de usuario de [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes);

    if((ARRAY_PROCESS_MEMORY[pid]->partition->size) >= (physical_address + 4)) {
        log_warning(MODULE_LOGGER, "Bytes recibidos para el proceso (PID:TID) (%u:%u) supera el limite de particion", pid, tid);
        return -1;
    }
    
    log_info(MINIMAL_LOGGER, "## Lectura - (PID:TID) - (%u:%u) - Dir. Fisica: %zu - Tamaño: %zu", pid, tid, physical_address, bytes);

    return 0;

}

int write_memory(t_Payload *payload) {

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;
    size_t physical_address;
    void *data;
    size_t bytes;

    payload_remove(payload, &pid, sizeof(pid));
    payload_remove(payload, &tid, sizeof(tid));
    size_deserialize(payload, &physical_address);
    data_deserialize(payload, &data, &bytes);

    log_info(MODULE_LOGGER, "[%d] Se recibe escritura en espacio de usuario de [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes);
    
    if((ARRAY_PROCESS_MEMORY[pid]->partition->size) >= (physical_address + 4)) {
        log_warning(MODULE_LOGGER, "Bytes recibidos para el proceso (PID:TID) (%u:%u) supera el limite de particion", pid, tid);
        if(send_header(WRITE_REQUEST_HEADER, CLIENT_CPU->fd_client)) {
            // TODO
        }
        return -1;
    }

    if(send_header(WRITE_REQUEST_HEADER, CLIENT_CPU->fd_client)) {
        // TODO
        return -1;
    }

    log_info(MINIMAL_LOGGER, "## Escritura - (PID:TID) - (%u:%u) - Dir. Fisica: %zu> - Tamaño: %zu", pid, tid, physical_address, bytes);

    return 0;
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

int attend_memory_dump(int fd_client, t_Payload *payload) {
    /*
    t_FS_Data* data = malloc(sizeof(t_FS_Data));
    if (data == NULL) {
        printf("No se pudo asignar memoria para t_FS_Data.\n");
        return -1;
    }

    payload_remove(payload, &(data->pid), sizeof(t_PID));
    payload_remove(payload, &(data->tid), sizeof(t_TID));
    */

    t_PID pid;
    t_TID tid;
    time_t current_time = time(NULL);

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));

    log_info(MODULE_LOGGER, "[%d] Kernel: Se recibe solicitud de volcado de memoria de [Cliente] %s [PID: %u - TID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid);

    char *namefile = string_new();
    sprintf(namefile, "<%u><%u><%ld>.dmp", pid, tid, (long)current_time);
    if(namefile == NULL) {
        printf("No se pudo generar el nombre del archivo.");
        free(namefile); 
       // free(data);
        return -1;
    }

    void* position = (void *)(((uint8_t *) MAIN_MEMORY) + ARRAY_PROCESS_MEMORY[pid]->partition->base);

    
	t_Connection connection_fileSystem = (t_Connection) {.client_type = MEMORY_PORT_TYPE, .server_type = FILESYSTEM_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_FILESYSTEM"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_FILESYSTEM")};

    if(send_memory_dump(namefile, position, ARRAY_PROCESS_MEMORY[pid]->size ,connection_fileSystem.fd_connection)) {
        printf("[DUMP]No se pudo enviar el paquete a FileSystem por la peticion PID:<%u> TID:<%u>.",pid, tid);
        free(namefile); 
        return -1;
    }

    if(receive_expected_header(MEMORY_DUMP_HEADER, connection_fileSystem.fd_connection)) {
        printf("[DUMP] Filesystem no pudo resolver la peticion por el PID:<%u> TID:<%u>.",pid, tid);
        free(namefile); 
        return -1;
    }

    log_info(MINIMAL_LOGGER, "## Memory Dump solicitado - (PID:TID) - (%u:%u)", pid, tid);

    return 0;
}

void seek_cpu_context(t_Payload *payload) {

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;

    payload_remove(payload, &pid, sizeof(t_PID));
    payload_remove(payload, &tid, sizeof(t_TID));

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de contexto de ejecución de [Cliente] %s [PID: %u - TID: %u]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid);

    if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
        return;
    }

    if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el hilo %u:%u", pid, tid);
        return;
    }

    t_Exec_Context context;
    context.cpu_registers = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers;
    context.base = ARRAY_PROCESS_MEMORY[pid]->partition->base;
    context.limit = ARRAY_PROCESS_MEMORY[pid]->size;

    if(send_exec_context(context, CLIENT_CPU->fd_client) == (-1)) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo enviar el contexto del proceso %d", pid);
        return;
    }
    
    log_info(MINIMAL_LOGGER, "## Contexto Solicitado - (PID:TID) - (%u:%u)", pid, tid);

    return;
}

void update_cpu_context(t_Payload *payload) {

    usleep(RESPONSE_DELAY * 1000);

    t_Exec_Context context;
    t_PID pid;
    t_TID tid;

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));
    exec_context_deserialize(payload, &(context));

    log_info(MODULE_LOGGER, "[%d] Se recibe actualización de contexto de ejecución de [Cliente] %s [PID: %u - TID: %u]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid);

    if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] == NULL) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo encontrar el hilo (PID:TID): (%u:%u)", pid, tid);
        return;
    }

    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers = context.cpu_registers;
    
    log_info(MINIMAL_LOGGER, "## Contexto Actualizado - (PID:TID) - (%u:%u)", pid, tid);

    return;
}

void free_memory(void) {
    int status;

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