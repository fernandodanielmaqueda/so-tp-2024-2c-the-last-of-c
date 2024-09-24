
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "memoria.h"

char *MODULE_NAME = "memoria";

t_log *MODULE_LOGGER = NULL;
char *MODULE_LOG_PATHNAME = "memoria.log";

t_config *MODULE_CONFIG = NULL;
char *MODULE_CONFIG_PATHNAME = "memoria.config";

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

int module(int argc, char* argv[]) {

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

    // TEMPORAL PARA EL CHECKPOINT 1: DESPUÉS BORRAR
    pthread_t thread_connection_filesystem;
    pthread_create(&thread_connection_filesystem, NULL, (void *(*)(void *)) client_thread_connect_to_server, &TEMPORAL_CONNECTION_FILESYSTEM);

    initialize_sockets();

    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

    listen_cpu();

	//finish_threads();
    free_memory();
	finish_sockets();
	//finish_configs();
	finish_loggers();
    finish_global_variables();

   return EXIT_SUCCESS;
}

void initialize_global_variables(void) {
    pthread_mutex_init(&(SHARED_LIST_CLIENTS_KERNEL.mutex), NULL);
    pthread_mutex_init(&(SHARED_LIST_CONNECTIONS_FILESYSTEM.mutex), NULL);
    pthread_mutex_init(&(MUTEX_PARTITION_TABLE), NULL);
    pthread_mutex_init(&(MUTEX_ARRAY_PROCESS_MEMORY), NULL);
}

void finish_global_variables(void) {
    pthread_mutex_destroy(&(SHARED_LIST_CLIENTS_KERNEL.mutex));
    pthread_mutex_destroy(&(SHARED_LIST_CONNECTIONS_FILESYSTEM.mutex));
    pthread_mutex_destroy(&(MUTEX_PARTITION_TABLE));
    pthread_mutex_destroy(&(MUTEX_ARRAY_PROCESS_MEMORY));
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
    TEMPORAL_CONNECTION_FILESYSTEM = (t_Connection) {.client_type = MEMORY_PORT_TYPE, .server_type = FILESYSTEM_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_FILESYSTEM"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_FILESYSTEM")};

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
    for (register e_Memory_Management_Scheme memory_management_scheme = 0; memory_management_scheme < memory_management_schemes_number; memory_management_scheme++)
        if (strcmp(MEMORY_MANAGEMENT_SCHEMES[memory_management_scheme].name, name) == 0) {
            *destination = memory_management_scheme;
            return 0;
        }

    return -1;
}

int memory_allocation_algorithm_find(char *name, e_Memory_Allocation_Algorithm *destination) {
    if(name == NULL || destination == NULL)
        return -1;
    
    size_t memory_allocation_algorithms_number = sizeof(MEMORY_ALLOCATION_ALGORITHMS) / sizeof(MEMORY_ALLOCATION_ALGORITHMS[0]);
    for (register e_Memory_Allocation_Algorithm memory_allocation_algorithm = 0; memory_allocation_algorithm < memory_allocation_algorithms_number; memory_allocation_algorithm++)
        if (strcmp(MEMORY_ALLOCATION_ALGORITHMS[memory_allocation_algorithm].name, name) == 0) {
            *destination = memory_allocation_algorithm;
            return 0;
        }

    return -1;
}

void listen_kernel(int fd_client) {

    t_Package* package;
    int result = 0;

    if(package_receive(&package, fd_client)) {
        
        switch(package->header) {
            
            case PROCESS_CREATE_HEADER:
                log_info(MODULE_LOGGER, "[%d] KERNEL: Creacion proceso nuevo recibido.", fd_client);
                result = create_process(&(package->payload));
                send_return_value_with_header(PROCESS_CREATE_HEADER, result, fd_client);
                break;
            
            case PROCESS_DESTROY_HEADER:
                log_info(MODULE_LOGGER, "[%d] KERNEL: Finalizar proceso recibido.", fd_client);
                result = kill_process(&(package->payload));
                send_return_value_with_header(PROCESS_DESTROY_HEADER, result, fd_client);
                break;
           
            case THREAD_CREATE_HEADER:
                log_info(MODULE_LOGGER, "[%d] KERNEL: Creacion hilo nuevo recibido.", fd_client);
                result = create_thread(&(package->payload));
                send_return_value_with_header(THREAD_CREATE_HEADER, result, fd_client);
                break;
             
            case THREAD_DESTROY_HEADER:
                log_info(MODULE_LOGGER, "[%d] KERNEL: Finalizar hilo recibido.", fd_client);
                result = kill_thread(&(package->payload));
                send_return_value_with_header(THREAD_DESTROY_HEADER, result, fd_client);
                break;
                
            case MEMORY_DUMP_HEADER:
                log_info(MODULE_LOGGER, "[%d] KERNEL: Finalizar hilo recibido.", fd_client);
                //result = treat_memory_dump(&(package->payload));
                send_return_value_with_header(MEMORY_DUMP_HEADER, result, fd_client);
                break;

            default:
                log_warning(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
                break;

        }

        package_destroy(package);
    }

	return;
}

    /*
    t_Package* package;
    
    while(1) {
        if(package_receive(&package, CLIENT_KERNEL->fd_client)) {
            // TODO

            pthread_cancel(SERVER_MEMORY.thread_server);
            pthread_join(SERVER_MEMORY.thread_server, NULL);
            close(SERVER_MEMORY.fd_listen);

            // ESTE ES EL HILO PRINCIPAL
            // pthread_cancel(CLIENT_KERNEL->thread_client_handler);
            // pthread_join(CLIENT_KERNEL->thread_client_handler, NULL);
            close(CLIENT_KERNEL->fd_client);

            pthread_cancel(CLIENT_CPU->thread_client_handler);
            pthread_join(CLIENT_CPU->thread_client_handler, NULL);
            close(CLIENT_CPU->fd_client);
            exit(EXIT_FAILURE);
        }
        switch(package->header) {
            case PROCESS_CREATE_HEADER:
                log_info(MODULE_LOGGER, "KERNEL: Proceso nuevo recibido.");
                create_process(&(package->payload));
                break;
                
            case PROCESS_DESTROY_HEADER:
                log_info(MODULE_LOGGER, "KERNEL: Proceso finalizado recibido.");
                kill_process(&(package->payload));
                break;
            
            default:
                log_warning(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
                break;
        }
        package_destroy(package);
    }
}

    */
int create_process(t_Payload *payload) {

    int result = 0;

    t_Memory_Process *new_process = malloc(sizeof(t_Memory_Process));
    if(new_process == NULL) {
        // TODO
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo proceso.", sizeof(t_Memory_Process));
        result = 1;
        return result;
    }

    payload_remove(payload, &(new_process->pid), sizeof(((t_Memory_Process *)0)->pid));
    size_deserialize(payload, &(new_process->size));

    //ASIGNAR PARTICION
    pthread_mutex_lock(&MUTEX_PARTITION_TABLE);
    bool unavailable = true;
    size_t count = list_size(PARTITION_TABLE);
    t_Partition* partition;
    if(count != 0) partition = list_get(PARTITION_TABLE, 0);

    switch(MEMORY_MANAGEMENT_SCHEME){

        case FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        {
            for (size_t i = 0; i < count; i++)
            {
                partition = list_get(PARTITION_TABLE, i);
                if ((partition->occupied == false) && (partition->size >= new_process->size)){
                    unavailable = false;
                    i = count;
                }
            }
            break;
        }

        case DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        {
            t_Partition* aux_partition;
            int location = 0;
            
            switch(MEMORY_ALLOCATION_ALGORITHM){
                case FIRST_FIT_MEMORY_ALLOCATION_ALGORITHM:
                {
                    for (size_t i = 0; i < count; i++)
                    {
                        partition = list_get(PARTITION_TABLE, i);
                        if ((partition->occupied == false) && (partition->size >= new_process->size)){
                            unavailable = false;
                            i = count;
                        }
                    }
                    break;
                }

                case BEST_FIT_MEMORY_ALLOCATION_ALGORITHM:
                {
                    for (size_t i = 0; i < count; i++)
                    {
                        aux_partition = list_get(PARTITION_TABLE, i);
                        if ((aux_partition->occupied == false) && (aux_partition->size >= new_process->size) && ((i == 0) || (aux_partition->size < partition->size))){
                            unavailable = false;
                            location = i;
                            partition = aux_partition;
                            break;
                        }
                    }
                    break;
                }

                case WORST_FIT_MEMORY_ALLOCATION_ALGORITHM:
                {
                    for (size_t i = 0; i < count; i++)
                    {
                        aux_partition = list_get(PARTITION_TABLE, i);
                        if ((aux_partition->occupied == false) && (aux_partition->size >= new_process->size) && ((i == 0) || (aux_partition->size > partition->size))){
                            unavailable = false;
                            location = i;
                            partition = aux_partition;
                            break;
                        }
                    }
                    break;
                }
            }
            //REALIZO EL SPLIT DE LA PARTICION (si es requerido)
            if((partition->size != new_process->size) && !(unavailable)) result = split_partition(location, new_process->size);

            break;
        }
    }

    if(unavailable){
        log_debug(MODULE_LOGGER, "[ERROR] No hay particiones disponibles para el pedido del proceso %d.\n", new_process->pid);
        free(new_process);
                result = 1;
    }
    else{
        log_debug(MODULE_LOGGER, "[OK] Particion asignada para el pedido del proceso %d.\n", new_process->pid);
        partition->pid = new_process->pid;
        partition->occupied = true;
        new_process->partition = partition;
        pthread_mutex_init(&(new_process->mutex_array_memory_threads), NULL);
        new_process->tid_count = 0;
        new_process->array_memory_threads = NULL;

        if(add_element_to_array_process(new_process)){
            log_debug(MODULE_LOGGER, "[ERROR] No se pudo agregar nuevo proceso al listado para el pedido del proceso %d.\n", new_process->pid);
            free(new_process);
                    result = 1;
        }
    }
    
    pthread_mutex_unlock(&MUTEX_PARTITION_TABLE);

    
    log_info(MINIMAL_LOGGER, "## Proceso <Creado> - PID:<%u> - TAMAÑO:<%zu>.\n", new_process->pid, new_process->size);

    /*
    new_process->instructions_list = list_create();
    //new_process->pages_table = list_create();

    char *argument_path;
    t_Return_Value flag_relative_path;

    payload_remove(payload, &(new_process->PID), sizeof(((t_Memory_Process *)0)->PID));
    text_deserialize(payload, &(argument_path));
    return_value_deserialize(payload, &flag_relative_path);

    // Genera la ruta hacia el archivo de pseudocódigo
    char *target_path;
    if(!flag_relative_path) {
        // Ruta absoluta
        target_path = argument_path;
    } else {
        // Ruta relativa
        target_path = malloc((INSTRUCTIONS_PATH[0] ? (strlen(INSTRUCTIONS_PATH) + 1) : 0) + strlen(argument_path) + 1);
        if(target_path == NULL) {
            log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para la ruta relativa.", (INSTRUCTIONS_PATH[0] ? (strlen(INSTRUCTIONS_PATH) + 1) : 0) + strlen(argument_path) + 1);
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
    }

    log_debug(MODULE_LOGGER, "Ruta hacia el archivo de pseudocódigo: %s", target_path);

    // Crea una lista de instrucciones a partir del archivo de pseudocódigo
    if(parse_pseudocode_file(target_path, new_process->instructions_list)) {
        // Envía un mensaje de error al Kernel en caso de no poder abrir el archivo de pseudocódigo
        if(send_return_value_with_header(PROCESS_CREATE_HEADER, 1, CLIENT_KERNEL->fd_client)) {
            // TODO

            pthread_cancel(SERVER_MEMORY.thread_server);
            pthread_join(SERVER_MEMORY.thread_server, NULL);
            close(SERVER_MEMORY.fd_listen);

            // ESTE ES EL HILO PRINCIPAL
            // pthread_cancel(CLIENT_KERNEL->thread_client_handler);
            // pthread_join(CLIENT_KERNEL->thread_client_handler, NULL);
            close(CLIENT_KERNEL->fd_client);

            pthread_cancel(CLIENT_CPU->thread_client_handler);
            pthread_join(CLIENT_CPU->thread_client_handler, NULL);
            close(CLIENT_CPU->fd_client);
            exit(EXIT_FAILURE);
        }
        return;
    }

    log_debug(MODULE_LOGGER, "Archivo de pseudocódigo encontrado: %s", target_path);
    free(target_path);

    log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Tamaño: <%d>", new_process->PID, 0);

    // TODO: Agregar MUTEX
    list_add(LIST_PROCESSES, new_process);

    // Envía respuesta OK a Kernel
    if(send_return_value_with_header(PROCESS_CREATE_HEADER, 0, CLIENT_KERNEL->fd_client)) {
        // TODO

        pthread_cancel(SERVER_MEMORY.thread_server);
        pthread_join(SERVER_MEMORY.thread_server, NULL);
        close(SERVER_MEMORY.fd_listen);

        // ESTE ES EL HILO PRINCIPAL
        // pthread_cancel(CLIENT_KERNEL->thread_client_handler);
        // pthread_join(CLIENT_KERNEL->thread_client_handler, NULL);
        close(CLIENT_KERNEL->fd_client);

        pthread_cancel(CLIENT_CPU->thread_client_handler);
        pthread_join(CLIENT_CPU->thread_client_handler, NULL);
        close(CLIENT_CPU->fd_client);
        exit(EXIT_FAILURE);
    }
    */

   return result;
}

int add_element_to_array_process (t_Memory_Process* process){
    pthread_mutex_lock(&MUTEX_ARRAY_PROCESS_MEMORY);
    ARRAY_PROCESS_MEMORY = realloc(ARRAY_PROCESS_MEMORY, sizeof(t_Memory_Process *) * (PID_COUNT + 1));    
    if (ARRAY_PROCESS_MEMORY == NULL) {
        log_warning(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el array de procesos", sizeof(t_Memory_Process *) * (PID_COUNT +1));
        return EXIT_FAILURE;
    }

    PID_COUNT++;
    ARRAY_PROCESS_MEMORY[PID_COUNT] = process;
    pthread_mutex_unlock(&MUTEX_ARRAY_PROCESS_MEMORY);

    return EXIT_SUCCESS;
}

int split_partition(int position, size_t size){
    
            t_Partition* old_partition = list_get(PARTITION_TABLE, position);
            t_Partition* new_partition = malloc(sizeof(t_Partition));
            if(new_partition == NULL) {
                fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
                exit(EXIT_FAILURE);
            }

            new_partition->size = (old_partition->size - size);
            new_partition->base = (old_partition->base + size);
            new_partition->occupied = false;
            old_partition->size = size;
            position++;

            list_add_in_index(PARTITION_TABLE, position, new_partition);

            return EXIT_SUCCESS;

}

int kill_process(t_Payload *payload) {
    
    int result = 0;
    t_PID pid;

    payload_remove(payload, &pid, sizeof(t_PID));
    size_t size = ARRAY_PROCESS_MEMORY[pid]->size;

    //Liberacion de particion
    pthread_mutex_lock(&MUTEX_PARTITION_TABLE);
    ARRAY_PROCESS_MEMORY[pid]->partition->occupied = false;
    ARRAY_PROCESS_MEMORY[pid]->partition->pid = -1;
    ARRAY_PROCESS_MEMORY[pid]->partition = NULL;
    ARRAY_PROCESS_MEMORY[pid]->size = -1;
    if(MEMORY_MANAGEMENT_SCHEME == DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME) result = verify_and_join_splited_partitions(pid);    
    pthread_mutex_unlock(&MUTEX_PARTITION_TABLE);

    //Liberacion de threads con sus struct
    free_threads(pid);
    
    log_info(MINIMAL_LOGGER, "## Proceso <Destruido> - PID:<%u> - TAMAÑO:<%zu>.\n", pid, size);
    
    return result;

    /*
    t_PID pid;
    payload_remove(payload, &pid, sizeof(pid));

    // TODO: Agregar MUTEX
    t_Process *process = list_remove_by_condition_with_comparation(LIST_PROCESSES, (bool (*)(void *, void *)) process_matches_pid, (void *) &pid);
    if(process == NULL) {
        log_warning(MODULE_LOGGER, "No se encontró un proceso con PID: %" PRIu16, pid);
    }
    else {

        // log_debug(MODULE_LOGGER, "PID: <%" PRIu16 "> - Tamaño: <%d>", pid, list_size(process->pages_table));

        char *instruction;
        t_Page *page;
        t_Frame *frame;
        
        for(int size = list_size(process->instructions_list); size > 0 ; size--) {
            instruction = list_remove(process->instructions_list, 0);
            free(instruction);
        }
        list_destroy(process->instructions_list);

        for(int size = list_size(process->pages_table); size > 0 ; size--) {
            page = list_remove(process->pages_table, 0);
            // TODO: Agregar MUTEX
            frame = list_get(LIST_FRAMES, (int) page->assigned_frame_number);
            // TODO: Agregar MUTEX
            list_add(LIST_FREE_FRAMES, frame);
            free(page);
        }
        list_destroy(process->pages_table);

        free(process);
    }

    // Enviar respuesta OK a Kernel
    if(send_header(PROCESS_DESTROY_HEADER, CLIENT_KERNEL->fd_client)) {
        // TODO

        pthread_cancel(SERVER_MEMORY.thread_server);
        pthread_join(SERVER_MEMORY.thread_server, NULL);
        close(SERVER_MEMORY.fd_listen);

        // ESTE ES EL HILO PRINCIPAL
        // pthread_cancel(CLIENT_KERNEL->thread_client_handler);
        // pthread_join(CLIENT_KERNEL->thread_client_handler, NULL);
        close(CLIENT_KERNEL->fd_client);

        pthread_cancel(CLIENT_CPU->thread_client_handler);
        pthread_join(CLIENT_CPU->thread_client_handler, NULL);
        close(CLIENT_CPU->fd_client);
        exit(EXIT_FAILURE);
    }
    */
}

int verify_and_join_splited_partitions(t_PID pid){

    int position = -1;
    t_Partition* partition;
    size_t count = list_size(PARTITION_TABLE);
    for (size_t i = 0; i < count; i++)
    {
        partition = list_get(PARTITION_TABLE, i);
        if(partition->pid == pid){
            position = i;
            break;
        }
    }

    if(position == (-1)) return 1;
    if((position != 0) && (position != (count-1))){ //No es 1er ni ultima posicion
        t_Partition* aux_partition_left;
        t_Partition* aux_partition_right;
        aux_partition_left = list_get(PARTITION_TABLE, (position -1));
        aux_partition_right = list_get(PARTITION_TABLE, (position +1));
        if(aux_partition_right->occupied == false){
            partition->size += aux_partition_right->size;
            //free(aux_partition_right);
            list_remove_and_destroy_element(PARTITION_TABLE, (position +1), free);
        }
        if(aux_partition_left->occupied == false){
            aux_partition_left->size += partition->size;
            //free(partition);
            list_remove_and_destroy_element(PARTITION_TABLE, position, free);
        }
        return 0;
    }

    if((position != 0) && (position == (count -1))){ //Es ultima posicion --> contempla que el len > 1
        t_Partition* aux_partition_left;
        aux_partition_left = list_get(PARTITION_TABLE, (position -1));
        if(aux_partition_left->occupied == false){
            aux_partition_left->size += partition->size;
            //free(partition);
            list_remove_and_destroy_element(PARTITION_TABLE, position, free);
        }
        return 0;
    }

    if((position == 0) && (position != (count - 1))){ //Es primer posicion --> contempla que el len > 1
        t_Partition* aux_partition_right;
        aux_partition_right = list_get(PARTITION_TABLE, (position +1));
        if(aux_partition_right->occupied == false){
            partition->size += aux_partition_right->size;
            //free(aux_partition_right);
            list_remove_and_destroy_element(PARTITION_TABLE, (position +1), free);
        }
        return 0;
    }
    
    return 0;
}

int create_thread(t_Payload *payload) {

    t_PID pid;
    char* path_instructions;
    int result = 0;

    t_Memory_Thread *new_thread = malloc(sizeof(t_Memory_Thread));
    if(new_thread == NULL) {
        // TODO
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo thread.", sizeof(t_Memory_Thread));
        return EXIT_FAILURE;
    }
    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(new_thread->tid), sizeof(((t_Memory_Thread *)0)->tid));
    text_deserialize(payload, &path_instructions);

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

    //inicializar instrucciones
    result = parse_pseudocode_file(path_instructions, &(new_thread->array_instructions), &(new_thread->instructions_count));
    if (result){
            for (size_t i = 0; i < new_thread->instructions_count; i++) {
                free(new_thread->array_instructions[i]);
            }
            free(new_thread->array_instructions);
            free(new_thread);
            return EXIT_FAILURE;
    }

    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads = realloc(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads, 
                                                    sizeof(t_Memory_Thread *) * (ARRAY_PROCESS_MEMORY[pid]->tid_count + 1)); 
    if (ARRAY_PROCESS_MEMORY == NULL) {
        log_warning(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el array de threads.", 
                                    sizeof(t_Memory_Thread *) * (ARRAY_PROCESS_MEMORY[pid]->tid_count  +1));
            for (size_t i = 0; i < new_thread->instructions_count; i++) {
                free(new_thread->array_instructions[i]);
            }
            free(new_thread->array_instructions);
            free(new_thread);
        return EXIT_FAILURE;
    }

    ARRAY_PROCESS_MEMORY[pid]->tid_count++;
    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[new_thread->tid] = new_thread;

    return EXIT_SUCCESS;
}

int kill_thread(t_Payload *payload) {
    t_PID pid;
    t_TID tid;

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));

    //Free instrucciones
        if( ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] != NULL){
            for (size_t y = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count; 0 < y; y--)
            {
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[y]);
            }
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions);
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]);
        }

    return EXIT_SUCCESS;
}

int parse_pseudocode_file(char *path, char*** array_instruction, t_PC* count) {
    FILE* file;
    if ((file = fopen(path, "r")) == NULL) {
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
                log_warning(MODULE_LOGGER, "Funcion getline: %s", strerror(errno));
                free(line);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            // Se terminó de leer el archivo
            break;
        }
        // Ignora líneas en blanco
        subline = strip_whitespaces(line);
        if(*subline) { // Se leyó una línea con contenido
    
            *array_instruction = realloc(*array_instruction, (*count + 1) * sizeof(char *));
            if (*array_instruction == NULL) {
                perror("[Error] realloc memory for array fallo.");
                free(line);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            (*array_instruction)[*count] = strdup(subline);
            if ((*array_instruction)[*count] == NULL) {
                perror("[Error] malloc memory for string fallo.");
                free(line);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            (*count)++;
        }
    }
    free(line);       
    fclose(file);
    return EXIT_SUCCESS;
}

void listen_cpu(void) {
    t_Package *package;

    while(1) {

        if(package_receive(&package, CLIENT_CPU->fd_client)) {
            // TODO

            pthread_cancel(SERVER_MEMORY.thread_server);
            pthread_join(SERVER_MEMORY.thread_server, NULL);
            close(SERVER_MEMORY.fd_listen);

            /*
            pthread_cancel(CLIENT_KERNEL->thread_client_handler);
            pthread_join(CLIENT_KERNEL->thread_client_handler, NULL);
            close(CLIENT_KERNEL->fd_client);
            */

            
            // ES ESTE HILO
            // pthread_cancel(CLIENT_CPU->thread_client_handler);
            // pthread_join(CLIENT_CPU->thread_client_handler, NULL);
            close(CLIENT_CPU->fd_client);
            exit(EXIT_FAILURE);
        }

        switch (package->header) {
            case INSTRUCTION_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de instruccion recibido.");
                seek_instruccion(&(package->payload));
                package_destroy(package);
                break;
                
            case READ_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de lectura recibido.");
                if(read_memory(&(package->payload), CLIENT_CPU->fd_client))send_return_value_with_header(READ_REQUEST_HEADER, 1, CLIENT_CPU->fd_client);
                //read_memory(&(package->payload), CLIENT_CPU->fd_client);
                package_destroy(package);
                break;
                
            case WRITE_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de escritura recibido.");
                int result_write = write_memory(&(package->payload));
                send_return_value_with_header(WRITE_REQUEST_HEADER, result_write, CLIENT_CPU->fd_client);
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

    if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] == NULL){
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

    if(send_text_with_header(INSTRUCTION_REQUEST_HEADER, instruccionBuscada, CLIENT_CPU->fd_client)){
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
    size_t bytes = 4;
    
    payload_remove(payload, &pid, sizeof(t_PID));
    payload_remove(payload, &tid, sizeof(t_TID));
    size_deserialize(payload, &physical_address);
    
    if(bytes > 4){
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de 4 bytes(BYTES: <%zd>).\n", pid, tid, bytes);
        return EXIT_FAILURE;
    }
    
    if((ARRAY_PROCESS_MEMORY[pid]->partition->size) >= (physical_address + 4)){
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de particion.\n", pid, tid);
        return EXIT_FAILURE;
    }
    
    void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

    t_Package *package = package_create_with_header(READ_REQUEST_HEADER);
    if(payload_add(&(package->payload), &pid, sizeof(pid))) {
        package_destroy(package);
        return EXIT_FAILURE;
    }
    if(payload_add(&(package->payload), &tid, sizeof(tid))) {
        package_destroy(package);
        return EXIT_FAILURE;
    }
    if(payload_add(&(package->payload), posicion, bytes)) {
        package_destroy(package);
        return EXIT_FAILURE;
    }
    if(size_serialize(&(package->payload), bytes)) {
        package_destroy(package);
        return EXIT_FAILURE;
    }
    if(package_send(package, socket)) {
        package_destroy(package);
        return EXIT_FAILURE;
    }
    package_destroy(package);

    
//FIX REQUIRED: se escribe 4 bytes segun definicion... se recibe menos?
    log_info(MINIMAL_LOGGER, "## <Lectura> - (PID:<%u>) - (TID:<%u>) - Dir. Fisica: <%zu> - Tamaño: <%zu>.\n", pid, tid, physical_address, bytes);

    return EXIT_SUCCESS;

    /*
    t_Package *package = package_create_with_header(READ_REQUEST_HEADER);

    if(list_size(list_physical_addresses) == 1) { //En caso de que sea igual a una página
        pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
            payload_add(&(package->payload), posicion, bytes);
        pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
         //Actualizar pagina/TDP
        update_page(current_frame);
    }
    else { //En caso de que el contenido supere a 1 pagina
        size_t bytes_restantes = bytes;
        size_t bytes_inicial = PAGE_SIZE - (physical_address - (current_frame * PAGE_SIZE));
        
        for (int i = 1; i > list_size(list_physical_addresses); i++)
        {
            physical_address = *((size_t *) list_get(list_physical_addresses, i - 1));
            current_frame = physical_address / PAGE_SIZE;
            //Posicion de la proxima escritura
            posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

            if(i == 1)//Primera pagina
            {
                pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                    payload_add(&(package->payload), posicion, bytes_inicial);
                pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
                update_page(current_frame);
                bytes_restantes -= bytes_inicial;
            }
            if((i == list_size(list_physical_addresses)) && (i != 1))//Ultima pagina
            {
                pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                    payload_add(&(package->payload), posicion, bytes_restantes);
                pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
                update_page(current_frame);
            }
            if((i < list_size(list_physical_addresses)) && (i != 1))//Paginas del medio
            {
                pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                    payload_add(&(package->payload), posicion, PAGE_SIZE);
                pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
                update_page(current_frame);
                bytes_restantes -= PAGE_SIZE;
            }
            
        }
    }

    package_send(package, socket);
    package_destroy(package);
    */
}

int write_memory(t_Payload *payload) {
    
    usleep(RESPONSE_DELAY * 1000);
    
    t_PID pid;
    t_TID tid;
    size_t physical_address;
    size_t bytes = 4;
    
    payload_remove(payload, &pid, sizeof(t_PID));
    payload_remove(payload, &tid, sizeof(t_TID));
    size_deserialize(payload, &physical_address);
    //size_deserialize(payload, &bytes);
    //t_list *list_physical_addresses = list_create();
    //list_deserialize(payload, list_physical_addresses, size_deserialize_element);
    
    void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

    if(bytes > 4){
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de 4 bytes(BYTES: <%zd>).\n", pid, tid, bytes);
        return EXIT_FAILURE;
    }
    
    if((ARRAY_PROCESS_MEMORY[pid]->partition->size) >= (physical_address + 4)){
        log_debug(MODULE_LOGGER, "[ERROR] Bytes recibidos para el proceso PID-TID: <%d-%d> supera el limite de particion.\n", pid, tid);
        return EXIT_FAILURE;
    }

    data_deserialize(payload, posicion, &bytes);

//FIX REQUIRED: se escribe 4 bytes segun definicion... se recibe menos?
    log_info(MINIMAL_LOGGER, "## <Escritura> - (PID:<%u>) - (TID:<%u>) - Dir. Fisica: <%zu> - Tamaño: <%zu>.\n", pid, tid, physical_address, bytes);

    return EXIT_SUCCESS;
    
/*
    size_t physical_address = *((size_t *) list_get(list_physical_addresses, 0));
    void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);
    
    size_t current_frame = physical_address / PAGE_SIZE;

    log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Accion: <ESCRIBIR> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes);

//COMIENZA LA ESCRITURA
    if(list_size(list_physical_addresses) == 1) {//En caso de que sea igual a 1 página
        pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
            payload_remove(payload, posicion, bytes);
        pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
         //Actualizar pagina/TDP
        update_page(current_frame);
    }
    else{//En caso de que el contenido supere a 1 pagina
        size_t bytes_restantes = bytes;
        size_t bytes_inicial = PAGE_SIZE - (physical_address - (current_frame * PAGE_SIZE));
        
        for (int i = 1; i > list_size(list_physical_addresses); i++)
        {
            physical_address = *((size_t *) list_get(list_physical_addresses, i - 1));
            current_frame = physical_address / PAGE_SIZE;
            //Posicion de la proxima escritura
            posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

            if (i == 1)//Primera pagina
            {
                pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                    payload_remove(payload, posicion, bytes_inicial);
                pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
                update_page(current_frame);
                bytes_restantes -= bytes_inicial;
            }
            if ((i == list_size(list_physical_addresses)) && (i != 1))//Ultima pagina
            {
                pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                    payload_remove(payload, posicion, bytes_restantes);
                pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
                update_page(current_frame);
                bytes_restantes -= bytes_inicial;
            }
            if ((i < list_size(list_physical_addresses)) && (i != 1))//Paginas del medio
            {
                pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                    payload_remove(payload, posicion, PAGE_SIZE);
                pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
                update_page(current_frame);
                bytes_restantes -= PAGE_SIZE;
            }
            
        }
    }

    list_destroy_and_destroy_elements(list_physical_addresses, free);

    if(send_return_value_with_header(WRITE_REQUEST_HEADER, 0, socket)) {
        // TODO
        exit(EXIT_FAILURE);
    }
    */
}

void free_threads(int pid){

    for (size_t i = ARRAY_PROCESS_MEMORY[pid]->tid_count; 0 < i; i--)
    {
        //Free instrucciones
        if( ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i] != NULL){
            for (size_t y = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]->instructions_count; 0 < y; y--)
            {
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]->array_instructions[y]);
            }
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]->array_instructions);
                free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[i]);
        }
    }
    
    free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads);

}

int treat_memory_dump(t_Payload *payload){
    
    t_PID pid;
    t_TID tid;

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));
    time_t current_time = time(NULL);

    char* namefile = string_new();
    sprintf(namefile, "<%u><%u><%ld>.dmp", pid, tid, (long)current_time);
    if (namefile == NULL) {
        printf("No se pudo generar el nombre del archivo.");
        free(namefile); 
        return EXIT_FAILURE;
    }
    
    void *position = (void *)(((uint8_t *) MAIN_MEMORY) + ARRAY_PROCESS_MEMORY[pid]->partition->base);
/*
FIX REQUIRED
    if(send_memory_dump(namefile, position, connection_filesystem.fd_connection)){
        printf("[DUMP]No se pudo enviar el paquete a FileSystem por la peticion PID:<%u> TID:<%u>.",pid, tid.");
        free(namefile); 
        return EXIT_FAILURE;
    }
    //Checkiar como se recibe 
    if(receive_expected_header(MEMORY_DUMP_HEADER, connection_filesystem.fd_connection)){
        printf("[DUMP] Filesystem no pudo resolver la peticion por el PID:<%u> TID:<%u>.",pid, tid);
        free(namefile); 
        return EXIT_FAILURE;
    }
*/
    free(namefile);
    
    log_info(MINIMAL_LOGGER, "## Memory dump solicitado - (PID:<%u>) - (TID:<%u>).\n", pid, tid);

    return EXIT_SUCCESS;
}

void seek_cpu_context(t_Payload *payload){

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));

    if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] == NULL){
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo encontrar el hilo PID-TID: %d-%d.\n", pid, tid);
        return;
    }

    t_Exec_Context context;
    context.cpu_registers = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers;
    context.base = ARRAY_PROCESS_MEMORY[pid]->partition->base;
    context.limit = ARRAY_PROCESS_MEMORY[pid]->partition->size;

    if(send_exec_context(context, CLIENT_CPU->fd_client) == (-1)){
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo enviar el contexto del proceso %d.\n", pid);
        return;
    }
    
    log_info(MINIMAL_LOGGER, "## Contexto <Solicitado> - (PID:<%u>) - (TID:<%u>).\n", pid, tid);

    return;
}

void update_cpu_context(t_Payload *payload){

    usleep(RESPONSE_DELAY * 1000);

    t_Exec_Context context;
    t_PID pid;
    t_TID tid;

    payload_remove(payload, &(pid), sizeof(t_PID));
    payload_remove(payload, &(tid), sizeof(t_TID));
    exec_context_deserialize(payload, &(context));
    
    if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] == NULL){
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo encontrar el hilo PID-TID: %d-%d.\n", pid, tid);
        return;
    }

    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers = context.cpu_registers;
    
    log_info(MINIMAL_LOGGER, "## Contexto <Actualizado> - (PID:<%u>) - (TID:<%u>).\n", pid, tid);

    return;
}

void free_memory(){

    //Free particiones
    for (size_t i = 0; i < list_size(PARTITION_TABLE); i++)
    {
        t_Partition* partition = list_get(PARTITION_TABLE, i);
        free(partition);
    }


    for (size_t i = PID_COUNT; 0 < i; i--) //Free procesos
    {
        if(ARRAY_PROCESS_MEMORY[i]->size != (-1)) free_threads(i);
        
        pthread_mutex_destroy(&(ARRAY_PROCESS_MEMORY[i]->mutex_array_memory_threads));
        free(ARRAY_PROCESS_MEMORY[i]);
    }

    free(ARRAY_PROCESS_MEMORY);
    free(MAIN_MEMORY);
    
}