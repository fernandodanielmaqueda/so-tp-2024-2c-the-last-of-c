
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

t_list *PARTITION_TABLE;
pthread_mutex_t MUTEX_PARTITION_TABLE;

t_PID PID_COUNT = 0;
t_Memory_Process **ARRAY_PROCESS_MEMORY = NULL;
pthread_mutex_t MUTEX_ARRAY_PROCESS_MEMORY;

pthread_mutex_t MUTEX_FILESYSTEM_MEMDUMP;

int module(int argc, char* argv[]) {
    int status;

    MODULE_NAME = "memoria";
    MODULE_LOG_PATHNAME = "memoria.log";
    MODULE_CONFIG_PATHNAME = "memoria.config";

    PARTITION_TABLE = list_create();

	if(initialize_configs(MODULE_CONFIG_PATHNAME)) {
        // TODO
        exit(EXIT_FAILURE);
    }

    initialize_loggers();
    initialize_global_variables();

    SHARED_LIST_CLIENTS_KERNEL.list = list_create();
    SHARED_LIST_CONNECTIONS_FILESYSTEM.list = list_create();

    MAIN_MEMORY = (void *) malloc(MEMORY_SIZE);
    if(MAIN_MEMORY == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para la memoria principal.", MEMORY_SIZE);
        exit(EXIT_FAILURE);
    }

    memset(MAIN_MEMORY, 0, MEMORY_SIZE);

    initialize_sockets();

    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

    //listen_cpu();

	//finish_threads();
    free_memory();
	finish_sockets();
	//finish_configs();
	finish_loggers();
    finish_global_variables();

   return 0;
}

void initialize_global_variables(void) {
    int status;

    if((status = pthread_mutex_init(&(SHARED_LIST_CLIENTS_KERNEL.mutex), NULL))) {
        log_error_pthread_mutex_init(status);
        // TODO
    }

    if((status = pthread_mutex_init(&(SHARED_LIST_CONNECTIONS_FILESYSTEM.mutex), NULL))) {
        log_error_pthread_mutex_init(status);
        // TODO
    }

    if((status = pthread_mutex_init(&(MUTEX_PARTITION_TABLE), NULL))) {
        log_error_pthread_mutex_init(status);
        // TODO
    }

    if((status = pthread_mutex_init(&(MUTEX_ARRAY_PROCESS_MEMORY), NULL))) {
        log_error_pthread_mutex_init(status);
        // TODO
    }
    
    if((status = pthread_mutex_init(&(MUTEX_FILESYSTEM_MEMDUMP), NULL))) {
        log_error_pthread_mutex_init(status);
        // TODO
    }
}

void finish_global_variables(void) {
    int status;

    if((status = pthread_mutex_destroy(&(SHARED_LIST_CLIENTS_KERNEL.mutex)))) {
        log_error_pthread_mutex_destroy(status);
        // TODO
    }

    if((status = pthread_mutex_destroy(&(SHARED_LIST_CONNECTIONS_FILESYSTEM.mutex)))) {
        log_error_pthread_mutex_destroy(status);
        // TODO
    }

    if((status = pthread_mutex_destroy(&(MUTEX_PARTITION_TABLE)))) {
        log_error_pthread_mutex_destroy(status);
        // TODO
    }

    if((status = pthread_mutex_destroy(&(MUTEX_ARRAY_PROCESS_MEMORY)))) {
        log_error_pthread_mutex_destroy(status);
        // TODO
    }
    
    if((status = pthread_mutex_destroy(&(MUTEX_FILESYSTEM_MEMDUMP)))) {
        log_error_pthread_mutex_destroy(status);
        // TODO
    }
}

int read_module_config(t_config* MODULE_CONFIG) {

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

    switch(MEMORY_MANAGEMENT_SCHEME) { ///CREACION DE PARTICIONES INICIAL

        case FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        { 
            char **fixed_partitions = config_get_array_value(MODULE_CONFIG, "PARTICIONES");
            if(fixed_partitions == NULL) {
                fprintf(stderr, "%s: la clave PARTICIONES no tiene valor\n", MODULE_CONFIG_PATHNAME);
                // string_array_destroy(fixed_partitions); TODO: Ver si acepta que fixed_partitions sea NULL
                return -1;
            }

            char *end;
            size_t base = 0;
            t_Partition *new_partition;
            for(register unsigned int i = 0; fixed_partitions[i] != NULL; i++) {
                new_partition = malloc(sizeof(t_Partition));
                if(new_partition == NULL) {
                    fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
                    // TODO: Liberar la lista de particiones
                    string_array_destroy(fixed_partitions);
                    return -1;
                }

                new_partition->size = strtoul(fixed_partitions[i], &end, 10);
                if(!*(fixed_partitions[i]) || *end) {
                    fprintf(stderr, "%s: valor de la clave PARTICIONES invalido: el tamaño de la partición %u no es un número entero válido: %s\n", MODULE_CONFIG_PATHNAME, i, fixed_partitions[i]);
                    // TODO: Liberar la lista de particiones
                    string_array_destroy(fixed_partitions);
                    return -1;
                }

                new_partition->base = base;
                new_partition->occupied = false;
                new_partition->pid = -1;

                list_add(PARTITION_TABLE, new_partition);

                base += new_partition->size;
            }

            if(list_size(PARTITION_TABLE) == 0) {
                fprintf(stderr, "%s: valor de la clave PARTICIONES invalido\n", MODULE_CONFIG_PATHNAME);
                string_array_destroy(fixed_partitions);
                return -1;
            }

            string_array_destroy(fixed_partitions);
            break;
        }

        case DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        {
            t_Partition *new_partition = malloc(sizeof(t_Partition));
            if(new_partition == NULL) {
                fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
                return -1;
            }

            new_partition->size = MEMORY_SIZE;
            new_partition->base = 0;
            new_partition->occupied = false;
            new_partition->pid = -1;

            list_add(PARTITION_TABLE, new_partition);
            break;
        }

    }

    SERVER_MEMORY = (t_Server) {.server_type = MEMORY_PORT_TYPE, .clients_type = TO_BE_IDENTIFIED_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA")};
    // TEMPORAL_CONNECTION_FILESYSTEM = (t_Connection) {.client_type = MEMORY_PORT_TYPE, .server_type = FILESYSTEM_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_FILESYSTEM"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_FILESYSTEM")};

    INSTRUCTIONS_PATH = config_get_string_value(MODULE_CONFIG, "PATH_INSTRUCCIONES");
        if(INSTRUCTIONS_PATH[0]) {

            size_t length = strlen(INSTRUCTIONS_PATH);
            if(INSTRUCTIONS_PATH[length - 1] == '/') {
                INSTRUCTIONS_PATH[length - 1] = '\0';
            }

            DIR *dir = opendir(INSTRUCTIONS_PATH);
            if(dir == NULL) {
                fprintf(stderr, "%s: No se pudo abrir el directorio indicado en el valor de PATH_INSTRUCCIONES: %s\n", MODULE_CONFIG_PATHNAME, INSTRUCTIONS_PATH);
                // TODO
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
        return;
    }
    int result;

    if(package_receive(package, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al recibir paquete de [Cliente] %s", fd_client, PORT_NAMES[KERNEL_PORT_TYPE]);
        return;
    }
    log_trace(MODULE_LOGGER, "[%d] Se recibe paquete de [Cliente] %s", fd_client, PORT_NAMES[KERNEL_PORT_TYPE]);

    switch(package->header) {

        case PROCESS_CREATE_HEADER:
            log_info(MODULE_LOGGER, "[%d] KERNEL: Creacion proceso nuevo recibido.", fd_client);
            result = create_process(&(package->payload));
            send_result_with_header(PROCESS_CREATE_HEADER, result, fd_client);
            break;
        
        case PROCESS_DESTROY_HEADER:
            log_info(MODULE_LOGGER, "[%d] KERNEL: Finalizar proceso recibido.", fd_client);
            result = kill_process(&(package->payload));
            send_result_with_header(PROCESS_DESTROY_HEADER, result, fd_client);
            break;
        
        case THREAD_CREATE_HEADER:
            log_info(MODULE_LOGGER, "[%d] KERNEL: Creacion hilo nuevo recibido.", fd_client);
            result = create_thread(&(package->payload));
            send_result_with_header(THREAD_CREATE_HEADER, result, fd_client);
            break;
            
        case THREAD_DESTROY_HEADER:
            log_info(MODULE_LOGGER, "[%d] KERNEL: Finalizar hilo recibido.", fd_client);
            result = kill_thread(&(package->payload));
            send_result_with_header(THREAD_DESTROY_HEADER, result, fd_client);
            break;
            
        case MEMORY_DUMP_HEADER:
            log_info(MODULE_LOGGER, "[%d] KERNEL: Finalizar hilo recibido.", fd_client);
            result = treat_memory_dump(&(package->payload));
            if (result == (-1)) send_result_with_header(MEMORY_DUMP_HEADER, result, fd_client);
            break;

        default:
            log_warning(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
            break;

    }

    package_destroy(package);

	return;
}

int create_process(t_Payload *payload) {

    int status;

    t_Memory_Process *new_process = malloc(sizeof(t_Memory_Process));
    if(new_process == NULL) {
        // TODO
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo proceso.", sizeof(t_Memory_Process));
        return -1;
    }

    payload_remove(payload, &(new_process->pid), sizeof(((t_Memory_Process *)0)->pid));
    size_deserialize(payload, &(new_process->size));

    //ASIGNAR PARTICION
    if((status = pthread_mutex_lock(&MUTEX_PARTITION_TABLE))) {
        log_error_pthread_mutex_lock(status);
        // TODO
    }
        bool available = true;
        size_t index_partition;

        if(allocate_partition(&index_partition, new_process->size)) {
            available = false;
        }

        if(!available) {
            log_debug(MODULE_LOGGER, "[ERROR] No hay particiones disponibles para el pedido del proceso %d.\n", new_process->pid);
            free(new_process);
            return -1;
        }

        switch(MEMORY_MANAGEMENT_SCHEME) {

            case FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME: {
                break;
            }

            case DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME: {

                // Realiza el fraccionamiento de la particion (si es requerido)
                if(split_partition(index_partition, new_process->size)) {
                    return -1;
                }

                break;
            }
        }

        log_debug(MODULE_LOGGER, "[OK] Particion asignada para el pedido del proceso %d.\n", new_process->pid);
        t_Partition *partition = list_get(PARTITION_TABLE, index_partition);
        partition->pid = new_process->pid;
        partition->occupied = true;
        new_process->partition = partition;
        if((status = pthread_mutex_init(&(new_process->mutex_array_memory_threads), NULL))) {
            log_error_pthread_mutex_init(status);
            // TODO
        }
        new_process->tid_count = 0;
        new_process->array_memory_threads = NULL;

        if(add_element_to_array_process(new_process)) {
            log_debug(MODULE_LOGGER, "[ERROR] No se pudo agregar nuevo proceso al listado para el pedido del proceso %d.\n", new_process->pid);
            free(new_process);
            return -1;
        }
    
    if((status = pthread_mutex_unlock(&MUTEX_PARTITION_TABLE))) {
        log_error_pthread_mutex_unlock(status);
        // TODO
    }

    log_info(MINIMAL_LOGGER, "## Proceso <Creado> - PID:<%u> - TAMAÑO:<%zu>.\n", new_process->pid, new_process->size);

    return 0;
}

int allocate_partition(size_t *index_partition, size_t required_size) {
    if(index_partition == NULL) {
        errno = EINVAL;
        return -1;
    }

    bool available = false;
    t_Partition *partition, *aux_partition;

    switch(MEMORY_ALLOCATION_ALGORITHM) {
        case FIRST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                partition = list_get(PARTITION_TABLE, i);
                if((partition->occupied == false) && (partition->size >= required_size)) {
                    available = true;
                    *index_partition = i;
                    break;
                }
            }

            break;
        }

        case BEST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((aux_partition->occupied == false) && (aux_partition->size >= required_size)) {
                    if(!available) {
                        available = true;
                        partition = aux_partition;
                        *index_partition = i;
                    }
                    else {
                        if(aux_partition->size < partition->size) {
                            partition = aux_partition;
                            *index_partition = i;
                        }
                    }
                }
            }

            break;
        }

        case WORST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((aux_partition->occupied == false) && (aux_partition->size >= required_size)) {
                    if(!available) {
                        available = true;
                        partition = aux_partition;
                        *index_partition = i;
                    }
                    else {
                        if(aux_partition->size > partition->size) {
                            partition = aux_partition;
                            *index_partition = i;
                        }
                    }
                }
            }

            break;
        }
    }

    if(!available) {
        errno = ENOMEM;
        return -1;
    }

    return 0;
}

int add_element_to_array_process (t_Memory_Process* process) {
    int status;

    if((status = pthread_mutex_lock(&MUTEX_ARRAY_PROCESS_MEMORY))) {
        log_error_pthread_mutex_lock(status);
        // TODO
    }

    ARRAY_PROCESS_MEMORY = realloc(ARRAY_PROCESS_MEMORY, sizeof(t_Memory_Process *) * (PID_COUNT + 1));    
    if(ARRAY_PROCESS_MEMORY == NULL) {
        log_warning(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(t_Memory_Process *) * PID_COUNT, sizeof(t_Memory_Process *) * (PID_COUNT +1));
        return -1;
    }

    PID_COUNT++;
    ARRAY_PROCESS_MEMORY[PID_COUNT] = process;
    if((status = pthread_mutex_unlock(&MUTEX_ARRAY_PROCESS_MEMORY))) {
        log_error_pthread_mutex_unlock(status);
        // TODO
    }

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

int kill_process(t_Payload *payload) {
    
    int status;
    t_PID pid;

    payload_remove(payload, &pid, sizeof(t_PID));
    size_t size = ARRAY_PROCESS_MEMORY[pid]->size;

    //Liberacion de particion
    if((status = pthread_mutex_lock(&MUTEX_PARTITION_TABLE))) {
        log_error_pthread_mutex_lock(status);
        // TODO
    }
    ARRAY_PROCESS_MEMORY[pid]->partition->occupied = false;
    ARRAY_PROCESS_MEMORY[pid]->partition->pid = -1;
    ARRAY_PROCESS_MEMORY[pid]->partition = NULL;
    ARRAY_PROCESS_MEMORY[pid]->size = -1;
    if(MEMORY_MANAGEMENT_SCHEME == DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME)
        if(verify_and_join_splited_partitions(pid)) {
            return -1;
        }
    if((status = pthread_mutex_unlock(&MUTEX_PARTITION_TABLE))) {
        log_error_pthread_mutex_unlock(status);
        // TODO
    }

    //Liberacion de threads con sus struct
    free_threads(pid);
    
    log_info(MINIMAL_LOGGER, "## Proceso <Destruido> - PID:<%u> - TAMAÑO:<%zu>.\n", pid, size);
    
    return 0;
}

int verify_and_join_splited_partitions(t_PID pid) {

    int position = -1;
    t_Partition* partition;
    size_t count = list_size(PARTITION_TABLE);
    for(size_t i = 0; i < count; i++)
    {
        partition = list_get(PARTITION_TABLE, i);
        if(partition->pid == pid) {
            position = i;
            break;
        }
    }

    if(position == (-1))
        return 1;
    if((position != 0) && (position != (count-1))) { //No es 1er ni ultima posicion
        t_Partition* aux_partition_left;
        t_Partition* aux_partition_right;
        aux_partition_left = list_get(PARTITION_TABLE, (position -1));
        aux_partition_right = list_get(PARTITION_TABLE, (position +1));
        if(aux_partition_right->occupied == false) {
            partition->size += aux_partition_right->size;
            //free(aux_partition_right);
            list_remove_and_destroy_element(PARTITION_TABLE, (position +1), free);
        }
        if(aux_partition_left->occupied == false) {
            aux_partition_left->size += partition->size;
            //free(partition);
            list_remove_and_destroy_element(PARTITION_TABLE, position, free);
        }
        return 0;
    }

    if((position != 0) && (position == (count -1))) { //Es ultima posicion --> contempla que el len > 1
        t_Partition* aux_partition_left;
        aux_partition_left = list_get(PARTITION_TABLE, (position -1));
        if(aux_partition_left->occupied == false) {
            aux_partition_left->size += partition->size;
            //free(partition);
            list_remove_and_destroy_element(PARTITION_TABLE, position, free);
        }
        return 0;
    }

    if((position == 0) && (position != (count - 1))) { //Es primer posicion --> contempla que el len > 1
        t_Partition* aux_partition_right;
        aux_partition_right = list_get(PARTITION_TABLE, (position +1));
        if(aux_partition_right->occupied == false) {
            partition->size += aux_partition_right->size;
            //free(aux_partition_right);
            list_remove_and_destroy_element(PARTITION_TABLE, (position +1), free);
        }
        return 0;
    }
    
    return 0;
}

int create_thread(t_Payload *payload) {

    t_Memory_Thread *new_thread = malloc(sizeof(t_Memory_Thread));
    if(new_thread == NULL) {
        // TODO
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo thread.", sizeof(t_Memory_Thread));
        return -1;
    }

    t_PID pid;
    char *argument_path;

    payload_remove(payload, &pid, sizeof(t_PID));
    payload_remove(payload, &(new_thread->tid), sizeof(((t_Memory_Thread *)0)->tid));
    text_deserialize(payload, &argument_path);

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
        log_error(MODULE_LOGGER, "malloc: No se pudo reservar memoria para la ruta relativa.");
        exit(EXIT_FAILURE);
    }

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

    //inicializar instrucciones
    if(parse_pseudocode_file(target_path, &(new_thread->array_instructions), &(new_thread->instructions_count))) {
        for(size_t i = 0; i < new_thread->instructions_count; i++) {
            free(new_thread->array_instructions[i]);
        }
        free(new_thread->array_instructions);
        free(new_thread);
        return -1;
    }

    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads = realloc(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads, sizeof(t_Memory_Thread *) * (ARRAY_PROCESS_MEMORY[pid]->tid_count + 1)); 
    if(ARRAY_PROCESS_MEMORY == NULL) {
        log_warning(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el array de threads.", 
                                    sizeof(t_Memory_Thread *) * (ARRAY_PROCESS_MEMORY[pid]->tid_count  +1));
            for(size_t i = 0; i < new_thread->instructions_count; i++) {
                free(new_thread->array_instructions[i]);
            }
            free(new_thread->array_instructions);
            free(new_thread);
        return -1;
    }

    ARRAY_PROCESS_MEMORY[pid]->tid_count++;
    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[new_thread->tid] = new_thread;

    return 0;
}

int kill_thread(t_Payload *payload) {
    t_PID pid;
    t_TID tid;

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));

    //Free instrucciones
        if( ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] != NULL) {
            for(size_t y = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count; 0 < y; y--)
            {
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[y]);
            }
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions);
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]);
        }

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
    int status;

    t_Package *package = package_create();
    if(package == NULL) {
        // TODO
        return;
    }

    while(1) {

        if(package_receive(package, CLIENT_CPU->fd_client)) {
            log_error(MODULE_LOGGER, "[%d] Error al recibir paquete de [Cliente] %s", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type]);
            exit(EXIT_FAILURE);
        }
        log_trace(MODULE_LOGGER, "[%d] Se recibe paquete de [Cliente] %s", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type]);

        switch(package->header) {
            case INSTRUCTION_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de instruccion recibido.");
                seek_instruccion(&(package->payload));
                package_destroy(package);
                break;
                
            case READ_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de lectura recibido.");
                if(read_memory(&(package->payload), CLIENT_CPU->fd_client))
                    send_result_with_header(READ_REQUEST_HEADER, 1, CLIENT_CPU->fd_client);
                package_destroy(package);
                break;
                
            case WRITE_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de escritura recibido.");
                int result_write = write_memory(&(package->payload));
                send_result_with_header(WRITE_REQUEST_HEADER, result_write, CLIENT_CPU->fd_client);
                package_destroy(package);
                break;
                
            case EXEC_CONTEXT_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de registros recibido.");
                seek_cpu_context(&(package->payload));
                package_destroy(package);
                break;
                
            case EXEC_CONTEXT_UPDATE_HEADER:
                log_info(MODULE_LOGGER, "CPU: Actualizacion de registros recibido.");
                update_cpu_context(&(package->payload));
                package_destroy(package);
                break;
            
            default:
                log_warning(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
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

    payload_remove(payload, &pid, sizeof(t_PID));
    payload_remove(payload, &tid, sizeof(t_TID));
    payload_remove(payload, &pc, sizeof(t_PC));

    if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] == NULL) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo encontrar el hilo PID-TID: %d-%d.\n", pid, tid);
        return;
    }
    
    if(pc > ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count) {
        log_debug(MODULE_LOGGER, "[ERROR] El ProgramCounter supera la cantidad de instrucciones para el hilo PID-TID: %d-%d.\n", pid, tid);
        return;
    }

    if(pc < 0) {
        log_debug(MODULE_LOGGER, "[ERROR] El ProgramCounter es invalido para el hilo PID-TID: %d-%d.\n", pid, tid);
        return;
    }

    char* instruccionBuscada = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[pc];

    if(send_text_with_header(INSTRUCTION_REQUEST_HEADER, instruccionBuscada, CLIENT_CPU->fd_client)) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo enviar la instruccion del proceso %d.\n", pid);
        return;
    }

    //FIX REQUIRED --> <> de instruccion
    log_info(MINIMAL_LOGGER, "## Obtener instruccion - (PID:<%u>) - (TID:<%u>) - Instruccion: <%s>.\n", pid, tid, instruccionBuscada);

    return;
}

int read_memory(t_Payload *payload, int socket) {
    
    usleep(RESPONSE_DELAY * 1000);
    
    t_PID pid;
    t_TID tid;
    size_t physical_address;
    size_t bytes;
    
    payload_remove(payload, &pid, sizeof(pid));
    payload_remove(payload, &tid, sizeof(tid));
    size_deserialize(payload, &physical_address);
    size_deserialize(payload, &bytes);
    
    if(bytes > 4) {
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de 4 bytes(BYTES: <%zd>).\n", pid, tid, bytes);
        if(send_data_with_header(READ_REQUEST_HEADER, NULL, 0, socket)) {
            // TODO
        }
        return -1;
    }
    
    if((ARRAY_PROCESS_MEMORY[pid]->partition->size) >= (physical_address + 4)) {
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de particion.\n", pid, tid);
        if(send_data_with_header(READ_REQUEST_HEADER, NULL, 0, socket)) {
            // TODO
        }
        return -1;
    }
    
    void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

    if(send_data_with_header(READ_REQUEST_HEADER, posicion, bytes, socket)) {
        // TODO
        return -1;
    }
    
//FIX REQUIRED: se escribe 4 bytes segun definicion... se recibe menos?
    log_info(MINIMAL_LOGGER, "## <Lectura> - (PID:<%u>) - (TID:<%u>) - Dir. Fisica: <%zu> - Tamaño: <%zu>.\n", pid, tid, physical_address, bytes);

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
    
    void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

    if(bytes > 4) {
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de 4 bytes(BYTES: <%zd>).\n", pid, tid, bytes);
        if(send_header(WRITE_REQUEST_HEADER, CLIENT_CPU->fd_client)) {
            // TODO
        }
        return -1;
    }
    
    if((ARRAY_PROCESS_MEMORY[pid]->partition->size) >= (physical_address + 4)) {
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de particion.\n", pid, tid);
        if(send_header(WRITE_REQUEST_HEADER, CLIENT_CPU->fd_client)) {
            // TODO
        }
        return -1;
    }

    if(send_header(WRITE_REQUEST_HEADER, CLIENT_CPU->fd_client)) {
        // TODO
        return -1;
    }

//FIX REQUIRED: se escribe 4 bytes segun definicion... se recibe menos?
    log_info(MINIMAL_LOGGER, "## <Escritura> - (PID:<%u>) - (TID:<%u>) - Dir. Fisica: <%zu> - Tamaño: <%zu>.\n", pid, tid, physical_address, bytes);

    return 0;
}

void free_threads(int pid) {

    for(size_t i = ARRAY_PROCESS_MEMORY[pid]->tid_count; 0 < i; i--)
    {
        //Free instrucciones
        if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i] != NULL) {
            for(size_t y = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]->instructions_count; 0 < y; y--)
            {
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]->array_instructions[y]);
            }
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]->array_instructions);
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]);
        }
    }
    
    free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads);

}

int treat_memory_dump(t_Payload *payload) {
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

/*
    pthread_t hilo_FS;
    if (pthread_create(&hilo_FS, NULL, attend_memory_dump, (void*)data) != 0) {
        printf("No se pudo crear el hilo correspondiente con FileSystem --> PID-TID: <%u><%u>.\n", data->pid, data->tid);
        free(data->namefile);
        free(data);
        return -1;
    }
    pthread_detach(hilo_FS);
*/    

    int status = 0;
    if((status = pthread_mutex_lock(&MUTEX_FILESYSTEM_MEMDUMP))) {
        log_error_pthread_mutex_lock(status);
        // TODO
    }

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
    
    if((status = pthread_mutex_unlock(&MUTEX_FILESYSTEM_MEMDUMP))) {
        log_error_pthread_mutex_unlock(status);
        // TODO
    }
    
    log_info(MINIMAL_LOGGER, "## Memory dump solicitado - (PID:<%u>) - (TID:<%u>).\n", pid, tid);

    return 0;
}


void* attend_memory_dump(void* arg){
/* 
    t_FS_Data* data = (t_FS_Data*)arg;
    int result = 0;
    int status;

    if((status = pthread_mutex_lock(&MUTEX_FILESYSTEM_MEMDUMP))) {
        log_error_pthread_mutex_lock(status);
        // TODO
    }

 FIX REQUIRED
    if(send_memory_dump(data.namefile, data.position, connection_filesystem.fd_connection)) {
        printf("[DUMP]No se pudo enviar el paquete a FileSystem por la peticion PID:<%u> TID:<%u>.",data.pid, data.tid");
        free(namefile); 
        result = -1;
    }
    if(result = (-1)) // Notificar error a hilo kernel
    
    //Checkiar como se recibe 
    if(receive_expected_header(MEMORY_DUMP_HEADER, connection_filesystem.fd_connection)) {
        printf("[DUMP] Filesystem no pudo resolver la peticion por el PID:<%u> TID:<%u>.",pid, tid);
        free(namefile); 
        result = -1;
    }

    if((status = pthread_mutex_unlock(&MUTEX_FILESYSTEM_MEMDUMP))) {
        log_error_pthread_mutex_unlock(status);
        // TODO
    }

    // Notificar resultado a hilo kernel

*/  
    return 0;
}

void seek_cpu_context(t_Payload *payload) {

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;

    payload_remove(payload, &pid, sizeof(t_PID));
    payload_remove(payload, &tid, sizeof(t_TID));

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
    context.limit = ARRAY_PROCESS_MEMORY[pid]->partition->size;

    if(send_exec_context(context, CLIENT_CPU->fd_client) == (-1)) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo enviar el contexto del proceso %d.\n", pid);
        return;
    }
    
    log_info(MINIMAL_LOGGER, "## Contexto <Solicitado> - (PID:<%u>) - (TID:<%u>).\n", pid, tid);

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
    
    if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] == NULL) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo encontrar el hilo PID-TID: %d-%d.\n", pid, tid);
        return;
    }

    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers = context.cpu_registers;
    
    log_info(MINIMAL_LOGGER, "## Contexto <Actualizado> - (PID:<%u>) - (TID:<%u>).\n", pid, tid);

    return;
}

void free_memory(void) {
    int status;

    //Free particiones
    for(size_t i = 0; i < list_size(PARTITION_TABLE); i++)
    {
        t_Partition* partition = list_get(PARTITION_TABLE, i);
        free(partition);
    }

    for(size_t i = PID_COUNT; 0 < i; i--) //Free procesos
    {
        if(ARRAY_PROCESS_MEMORY[i]->size != (-1)) free_threads(i);
        
        if((status = pthread_mutex_destroy(&(ARRAY_PROCESS_MEMORY[i]->mutex_array_memory_threads)))) {
            log_error_pthread_mutex_destroy(status);
            // TODO
        }

        free(ARRAY_PROCESS_MEMORY[i]);
    }

    free(ARRAY_PROCESS_MEMORY);
    free(MAIN_MEMORY);
    
}