
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "memoria.h"

char *MODULE_NAME = "memoria";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "memoria.log";

t_config *MODULE_CONFIG;
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

t_PID PID_COUNT;
t_Memory_Process **ARRAY_PROCESS_MEMORY;
pthread_mutex_t MUTEX_ARRAY_PROCESS_MEMORY;

int module(int argc, char* argv[]) {

    PARTITION_TABLE = list_create();

	initialize_configs(MODULE_CONFIG_PATHNAME);
    initialize_loggers();
    initialize_global_variables();

    SHARED_LIST_CLIENTS_KERNEL.list = list_create();
    SHARED_LIST_CONNECTIONS_FILESYSTEM.list = list_create();

    MAIN_MEMORY = (void *) malloc(MEMORY_SIZE);
    memset(MAIN_MEMORY, 0, MEMORY_SIZE); //Llena de 0's el espacio de memoria

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
}

void finish_global_variables(void) {
    pthread_mutex_destroy(&(SHARED_LIST_CLIENTS_KERNEL.mutex));
    pthread_mutex_destroy(&(SHARED_LIST_CONNECTIONS_FILESYSTEM.mutex));
}

void read_module_config(t_config* MODULE_CONFIG) {

    if(!config_has_properties(MODULE_CONFIG, "PUERTO_ESCUCHA", "IP_FILESYSTEM", "PUERTO_FILESYSTEM", "TAM_MEMORIA", "PATH_INSTRUCCIONES", "RETARDO_RESPUESTA", "ESQUEMA", "ALGORITMO_BUSQUEDA", "PARTICIONES", "LOG_LEVEL", NULL)) {
        //fprintf(stderr, "%s: El archivo de configuración no tiene la propiedad/key/clave %s", MODULE_CONFIG_PATHNAME, "LOG_LEVEL");
        exit(EXIT_FAILURE);
    }

    char *string;

    string = config_get_string_value(MODULE_CONFIG, "ESQUEMA");
	if(memory_management_scheme_find(string, &MEMORY_MANAGEMENT_SCHEME)) {
		fprintf(stderr, "%s: No se reconoce el ESQUEMA", string);
		exit(EXIT_FAILURE);
	}

    string = config_get_string_value(MODULE_CONFIG, "ALGORITMO_BUSQUEDA");
	if(memory_allocation_algorithm_find(string, &MEMORY_ALLOCATION_ALGORITHM)) {
		fprintf(stderr, "%s: No se reconoce el ALGORITMO_BUSQUEDA", string);
		exit(EXIT_FAILURE);
	}

    MEMORY_SIZE = (size_t) config_get_int_value(MODULE_CONFIG, "TAM_MEMORIA");

    switch(MEMORY_MANAGEMENT_SCHEME) {

        case FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        { 
            char **fixed_partitions = config_get_array_value(MODULE_CONFIG, "PARTICIONES");
            if(fixed_partitions == NULL) {
                fprintf(stderr, "No se pudo obtener el valor de PARTICIONES");
                // string_array_destroy(fixed_partitions); TODO: Ver si acepta que fixed_partitions sea NULL
                exit(EXIT_FAILURE);
            }

            char *end;
            size_t base = 0;
            t_Partition *new_partition;
            for(register unsigned int i = 0; fixed_partitions[i] != NULL; i++) {
                new_partition = malloc(sizeof(t_Partition));
                if(new_partition == NULL) {
                    fprintf(stderr, "malloc: No se pudo reservar memoria para una particion");
                    // TODO: Liberar la lista de particiones
                    string_array_destroy(fixed_partitions);
                    exit(EXIT_FAILURE);
                }

                new_partition->size = strtoul(fixed_partitions[i], &end, 10);
                if(!*(fixed_partitions[i]) || *end) {
                    fprintf(stderr, "El tamaño de la partición %d no es un número entero válido: %s", i, fixed_partitions[i]);
                    // TODO: Liberar la lista de particiones
                    string_array_destroy(fixed_partitions);
                    exit(EXIT_FAILURE);
                }

                new_partition->base = base;
                new_partition->occupied = false;

                list_add(PARTITION_TABLE, new_partition);

                base += new_partition->size;
            }

            if(list_size(PARTITION_TABLE) == 0) {
                fprintf(stderr, "No se encontraron particiones fijas");
                string_array_destroy(fixed_partitions);
                exit(EXIT_FAILURE);
            }

            string_array_destroy(fixed_partitions);
            break;
        }

        case DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME:
        {
            t_Partition *new_partition = malloc(sizeof(t_Partition));
            if(new_partition == NULL) {
                fprintf(stderr, "malloc: No se pudo reservar memoria para una particion");
                exit(EXIT_FAILURE);
            }

            new_partition->size = MEMORY_SIZE;
            new_partition->base = 0;
            new_partition->occupied = false;

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
                fprintf(stderr, "No se pudo abrir el directorio de instrucciones.");
                // TODO
                exit(EXIT_FAILURE);
            }
            closedir(dir);
        }

    RESPONSE_DELAY = config_get_int_value(MODULE_CONFIG, "RETARDO_RESPUESTA");

    LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));
}

int memory_management_scheme_find(char *name, e_Memory_Management_Scheme *destination) {
    if(name == NULL || destination == NULL)
        return 1;
    
    size_t memory_management_schemes_number = sizeof(MEMORY_MANAGEMENT_SCHEMES) / sizeof(MEMORY_MANAGEMENT_SCHEMES[0]);
    for (register e_Memory_Management_Scheme memory_management_scheme = 0; memory_management_scheme < memory_management_schemes_number; memory_management_scheme++)
        if (strcmp(MEMORY_MANAGEMENT_SCHEMES[memory_management_scheme].name, name) == 0) {
            *destination = memory_management_scheme;
            return 0;
        }

    return 1;
}

int memory_allocation_algorithm_find(char *name, e_Memory_Allocation_Algorithm *destination) {
    if(name == NULL || destination == NULL)
        return 1;
    
    size_t memory_allocation_algorithms_number = sizeof(MEMORY_ALLOCATION_ALGORITHMS) / sizeof(MEMORY_ALLOCATION_ALGORITHMS[0]);
    for (register e_Memory_Allocation_Algorithm memory_allocation_algorithm = 0; memory_allocation_algorithm < memory_allocation_algorithms_number; memory_allocation_algorithm++)
        if (strcmp(MEMORY_ALLOCATION_ALGORITHMS[memory_allocation_algorithm].name, name) == 0) {
            *destination = memory_allocation_algorithm;
            return 0;
        }

    return 1;
}

void *listen_kernel(t_Client *new_client) {

	log_trace(MODULE_LOGGER, "Hilo receptor de [Cliente] Kernel [%d] iniciado", new_client->fd_client);

	// Borrar este while(1) (ciclo incluido) y reemplazarlo por la lógica necesaria para atender al cliente
	while(1) {
		getchar();
	}

	close(new_client->fd_client);

	return NULL;

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
            exit(1);
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
    */
}

void create_process(t_Payload *payload) {

    /*
    t_Process *new_process = malloc(sizeof(t_Process));
    if(new_process == NULL) {
        // TODO
        log_error(MODULE_LOGGER, "malloc: No se pudo reservar memoria para el nuevo proceso.");
        return;
    }

    new_process->instructions_list = list_create();
    //new_process->pages_table = list_create();

    char *argument_path;
    t_Return_Value flag_relative_path;

    payload_remove(payload, &(new_process->PID), sizeof(new_process->PID));
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
            exit(1);
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
        exit(1);
    }
    */
}

void kill_process(t_Payload *payload) {

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
        exit(1);
    }
    */
}

int parse_pseudocode_file(char *path, t_list *list_instruction) {

    FILE* file;
    if ((file = fopen(path, "r")) == NULL) {
        log_warning(MODULE_LOGGER, "%s: No se pudo abrir el archivo de pseudocodigo indicado.", path);
        return 1;
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
                exit(EXIT_FAILURE);
            }

            // Se terminó de leer el archivo
            break;
        }

        // Ignora líneas en blanco
        subline = strip_whitespaces(line);

        if(*subline) {
            // Se leyó una línea con contenido
            list_add(list_instruction, strdup(subline));
        }
    }

    free(line);       
    fclose(file);
    return 0;
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
            exit(1);
        }

        switch (package->header) {
            case INSTRUCTION_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de instruccion recibido.");
                seek_instruccion(&(package->payload));
                package_destroy(package);
                break;
                
            case READ_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de lectura recibido.");
                attend_read(&(package->payload), CLIENT_CPU->fd_client);
                package_destroy(package);
                break;
                
            case WRITE_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de escritura recibido.");
                attend_write(&(package->payload), CLIENT_CPU->fd_client);
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
    /*
    t_PID PID;
    t_PC PC;

    payload_remove(payload, &PID, sizeof(PID));
    payload_remove(payload, &PC, sizeof(PC));
    
    t_Process *procesoBuscado = list_find_by_condition_with_comparation(LIST_PROCESSES, (bool (*)(void *, void *)) process_matches_pid, (void *) &PID);

    char* instruccionBuscada = NULL;
    if(PC < list_size(procesoBuscado->instructions_list)) {
        instruccionBuscada = list_get(procesoBuscado->instructions_list, PC);
    }

    usleep(RESPONSE_DELAY * 1000);
    if(send_text_with_header(INSTRUCTION_REQUEST_HEADER, instruccionBuscada, CLIENT_CPU->fd_client)) {
        // TODO

        pthread_cancel(SERVER_MEMORY.thread_server);
        pthread_join(SERVER_MEMORY.thread_server, NULL);
        close(SERVER_MEMORY.fd_listen);

        pthread_cancel(CLIENT_KERNEL->thread_client_handler);
        pthread_join(CLIENT_KERNEL->thread_client_handler, NULL);
        close(CLIENT_KERNEL->fd_client);

        
        // ES ESTE HILO
        // pthread_cancel(CLIENT_CPU->thread_client_handler);
        // pthread_join(CLIENT_CPU->thread_client_handler, NULL);
        close(CLIENT_CPU->fd_client);
        exit(1);
    }
    log_info(MODULE_LOGGER, "Instruccion enviada.");¨
    */
}

void attend_read(t_Payload *payload, int socket) {
    /*
    t_PID pid;
    t_list *list_physical_addresses = list_create();
    size_t bytes;

    payload_remove(payload, &pid, sizeof(pid));
    list_deserialize(payload, list_physical_addresses, size_deserialize_element);
    size_deserialize(payload, &bytes);

    size_t physical_address = *((size_t *) list_get(list_physical_addresses, 0));
    
    void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

    log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Accion: <LEER> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes);

    size_t current_frame = physical_address / PAGE_SIZE;

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

void attend_write(t_Payload *payload, int socket) {
    /*
    t_PID pid;
    t_list *list_physical_addresses = list_create();
    size_t bytes;
    
    payload_remove(payload, &pid, sizeof(pid));
    list_deserialize(payload, list_physical_addresses, size_deserialize_element);
    size_deserialize(payload, &bytes);

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
        exit(1);
    }
    */
}

void free_memory(void){
    free(MAIN_MEMORY);
}