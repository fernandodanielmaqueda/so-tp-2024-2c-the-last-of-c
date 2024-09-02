
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "memoria.h"

char *MODULE_NAME = "memoria";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "memoria.log";

char *MODULE_CONFIG_PATHNAME = "memoria.config";

t_config *MODULE_CONFIG;

void *MAIN_MEMORY;

t_list *LIST_PROCESSES;
t_list *LIST_FRAMES;
t_list *LIST_FREE_FRAMES;

pthread_mutex_t MUTEX_MAIN_MEMORY;
pthread_mutex_t MUTEX_LIST_FREE_FRAMES;

size_t MEMORY_SIZE;
size_t PAGE_SIZE;
char *INSTRUCTIONS_PATH;
int RESPONSE_DELAY;

int module(int argc, char* argv[]) {

	initialize_configs(MODULE_CONFIG_PATHNAME);
    initialize_loggers();
    initialize_mutexes();
	initialize_semaphores();

    SHARED_LIST_CLIENTS_IO.list = list_create();

    MAIN_MEMORY = (void *) malloc(MEMORY_SIZE);
    memset(MAIN_MEMORY, 0, MEMORY_SIZE); //Llena de 0's el espacio de memoria
    LIST_PROCESSES = list_create();
    create_frames();

    initialize_sockets();

    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

    listen_kernel();

	//finish_threads();
    free_memory();
	finish_sockets();
	//finish_configs();
	finish_loggers();
	finish_semaphores();
	finish_mutexes();

   return EXIT_SUCCESS;
}

void initialize_mutexes(void) {
    pthread_mutex_init(&(SHARED_LIST_CLIENTS_IO.mutex), NULL);
    pthread_mutex_init(&MUTEX_MAIN_MEMORY, NULL);
    pthread_mutex_init(&MUTEX_LIST_FREE_FRAMES, NULL);
}

void finish_mutexes(void) {
    pthread_mutex_destroy(&(SHARED_LIST_CLIENTS_IO.mutex));
    pthread_mutex_destroy(&(MUTEX_MAIN_MEMORY));
    pthread_mutex_destroy(&(MUTEX_LIST_FREE_FRAMES));
}

void initialize_semaphores(void) {
    
}

void finish_semaphores(void) {
    
}

void read_module_config(t_config* MODULE_CONFIG) {
    SERVER_MEMORY = (t_Server) {.server_type = MEMORY_PORT_TYPE, .clients_type = TO_BE_IDENTIFIED_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA")};
    MEMORY_SIZE = (size_t) config_get_int_value(MODULE_CONFIG, "TAM_MEMORIA");
    PAGE_SIZE = (size_t) config_get_int_value(MODULE_CONFIG, "TAM_PAGINA");

    INSTRUCTIONS_PATH = config_get_string_value(MODULE_CONFIG, "PATH_INSTRUCCIONES");
        if(INSTRUCTIONS_PATH[0]) {

            size_t length = strlen(INSTRUCTIONS_PATH);
            if(INSTRUCTIONS_PATH[length - 1] == '/') {
                INSTRUCTIONS_PATH[length - 1] = '\0';
            }

            DIR *dir = opendir(INSTRUCTIONS_PATH);
            if(dir == NULL) {
                log_error(MODULE_LOGGER, "No se pudo abrir el directorio de instrucciones.");
                // TODO
                exit(EXIT_FAILURE);
            }
            closedir(dir);
        }

    RESPONSE_DELAY = config_get_int_value(MODULE_CONFIG, "RETARDO_RESPUESTA");
    LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));
}

void listen_kernel(void) {

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
                log_warning(MODULE_LOGGER, "%s: Header desconocido (%d)", "", package->header);
                break;
        }
        package_destroy(package);
    }
}

void create_process(t_Payload *payload) {

    t_Process *new_process = malloc(sizeof(t_Process));
    if(new_process == NULL) {
        // TODO
        log_error(MODULE_LOGGER, "malloc: No se pudo reservar memoria para el nuevo proceso.");
        return;
    }

    new_process->instructions_list = list_create();
    new_process->pages_table = list_create();

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
}

void kill_process(t_Payload *payload) {

    t_PID pid;
    payload_remove(payload, &pid, sizeof(pid));

    // TODO: Agregar MUTEX
    t_Process *process = list_remove_by_condition_with_comparation(LIST_PROCESSES, (bool (*)(void *, void *)) process_matches_pid, (void *) &pid);
    if(process == NULL) {
        log_warning(MODULE_LOGGER, "No se encontró un proceso con PID: %" PRIu16, pid);
    }
    else {

        log_debug(MODULE_LOGGER, "PID: <%" PRIu16 "> - Tamaño: <%d>", pid, list_size(process->pages_table));

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

            pthread_cancel(CLIENT_KERNEL->thread_client_handler);
            pthread_join(CLIENT_KERNEL->thread_client_handler, NULL);
            close(CLIENT_KERNEL->fd_client);

            
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
                
            case FRAME_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de frame recibido.");
                respond_frame_request(&(package->payload));
                package_destroy(package);
                break;
                
            case PAGE_SIZE_REQUEST_HEADER:
            {
                log_info(MODULE_LOGGER, "CPU: Pedido de tamaño de pagina recibido.");
                package_destroy(package);

                package = package_create_with_header(PAGE_SIZE_REQUEST_HEADER);
                size_serialize(&(package->payload), PAGE_SIZE);
                package_send(package, CLIENT_CPU->fd_client);
                package_destroy(package);

                break;
            }
            case RESIZE_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de tamaño de pagina recibido.");
                resize_process(&(package->payload));
                package_destroy(package);
                break;
                
            case READ_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de lectura recibido.");
                io_read_memory(&(package->payload), CLIENT_CPU->fd_client);
                package_destroy(package);
                break;
                
            case WRITE_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de escritura recibido.");
                io_write_memory(&(package->payload), CLIENT_CPU->fd_client);
                package_destroy(package);
                break;
                
            case COPY_REQUEST_HEADER:
                log_info(MODULE_LOGGER, "CPU: Pedido de escritura recibido.");
                copy_memory(&(package->payload), CLIENT_CPU->fd_client);
                package_destroy(package);
                break;
            
            default:
                log_warning(MODULE_LOGGER, "%s: Header desconocido (%d)", "", package->header);
                package_destroy(package);
                break;
        }
    }
}

void listen_io(t_Client *client) {
    t_Package *package;

    while(1) {
        if(package_receive(&package, client->fd_client)) {
            log_warning(MODULE_LOGGER, "Terminada conexion con [Cliente] Entrada/Salida");

            pthread_mutex_lock(&(SHARED_LIST_CLIENTS_IO.mutex));
                list_remove_by_condition_with_comparation(SHARED_LIST_CLIENTS_IO.list, (bool (*)(void *, void *)) client_matches_pthread, (void *) &(client->thread_client_handler));
            pthread_mutex_unlock(&(SHARED_LIST_CLIENTS_IO.mutex));

            close(client->fd_client);
            free(client);

            return;
        }
        switch(package->header) {

            case IO_STDIN_WRITE_MEMORY_HEADER:
                log_info(MODULE_LOGGER, "IO: Nueva peticion STDIN_IO (write) recibido.");
                io_write_memory(&(package->payload), client->fd_client);
                break;
            
            case IO_STDOUT_READ_MEMORY_HEADER:
                log_info(MODULE_LOGGER, "IO: Nueva peticion STDOUT_IO (read) recibido.");
                io_read_memory(&(package->payload), client->fd_client);
                break;
            
            case IO_FS_READ_MEMORY_HEADER:
                log_info(MODULE_LOGGER, "IO: Nueva peticion STDOUT_IO (write) recibido.");
                io_write_memory(&(package->payload), client->fd_client);
                break;
            
            case IO_FS_WRITE_MEMORY_HEADER:
                log_info(MODULE_LOGGER, "IO: Nueva peticion STDOUT_IO (read) recibido.");
                io_read_memory(&(package->payload), client->fd_client);
                break;
            
            default:
                log_warning(MODULE_LOGGER, "%s: Header desconocido (%d)", "", package->header);
                break;
        }
        package_destroy(package);
    }
}

bool process_matches_pid(t_Process *process, t_PID *pid) {
    return process->PID == *pid;
}

void seek_instruccion(t_Payload *payload) {
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
    log_info(MODULE_LOGGER, "Instruccion enviada.");
}

void create_frames(void) {
    LIST_FRAMES = list_create();
    LIST_FREE_FRAMES = list_create();

    size_t frame_quantity = MEMORY_SIZE / PAGE_SIZE;

    size_t offset = MEMORY_SIZE % PAGE_SIZE;
    if(offset != 0)
        frame_quantity++;

    for(size_t i = 0; i < frame_quantity; i++) {
        t_Frame *new_frame = malloc(sizeof(t_Frame));
        new_frame->frame_number = i;
        new_frame->assigned_page = NULL;
        list_add(LIST_FRAMES, new_frame);
        list_add(LIST_FREE_FRAMES, new_frame);
    }
}

void free_frames(void) {
    t_Frame *frame;

    for(int i = list_size(LIST_FRAMES); i > 0; i--)
    {
        frame = (t_Frame *) list_get(LIST_FRAMES, i - 1);
        free(frame);
    }

}

void respond_frame_request(t_Payload *payload) {
//Recibir parametros
    size_t page_number;
    t_PID pidProceso;

    size_deserialize(payload, &page_number);
    payload_remove(payload, &pidProceso, sizeof(pidProceso));

//Buscar frame
    t_Process *procesoBuscado = list_find_by_condition_with_comparation(LIST_PROCESSES, (bool (*)(void *, void *)) process_matches_pid, (void *) &pidProceso);
    
    size_t *frame_number = seek_frame_number_by_page_number(procesoBuscado->pages_table, page_number);
    if(frame_number != NULL)
        log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Pagina: <%zd> - Marco: <%zd>", pidProceso, page_number, *frame_number);
    else
        log_error(MODULE_LOGGER, "El numero de página <%zd> no existe en la tabla de paginas.", page_number);

//Respuesta
    usleep(RESPONSE_DELAY * 1000);
    
    t_Package* package = package_create_with_header(FRAME_REQUEST_HEADER);
    if(frame_number != NULL) {
        return_value_serialize(&(package->payload), 0);
        size_serialize(&(package->payload), *frame_number);
    }
    else {
        return_value_serialize(&(package->payload), 1);
    }
    package_send(package, CLIENT_CPU->fd_client);
}

size_t *seek_frame_number_by_page_number(t_list *page_table, size_t page_number) {
    t_Page *page;
    uint32_t size = list_size(page_table);

    for(size_t i = 0; i < size ; i++) {
        page = (t_Page *) list_get(page_table, i);
        if(page->page_number == page_number) {
            return &(page->assigned_frame_number);
        }
    }

    return NULL;
}

void io_read_memory(t_Payload *payload, int socket) {
    t_PID pid;
    t_list *list_physical_addresses = list_create();
    size_t bytes;

    payload_remove(payload, &pid, sizeof(pid));
    list_deserialize(payload, list_physical_addresses, size_deserialize_element);
    size_deserialize(payload, &bytes);

    char text_to_send[bytes + 1]; // Le agrego un '\0' al final por las dudas para asegurar de que el string se pueda imprimir
    size_t offset = 0;

    size_t physical_address = *((size_t *) list_get(list_physical_addresses, 0));
    
    log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Accion: <LEER> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes);

    t_Package* package = package_create_with_header(READ_REQUEST_HEADER);

    int size = list_size(list_physical_addresses);
    for (int i = 0; i < size; i++) {
        physical_address = *((size_t *) list_get(list_physical_addresses, i));
        size_t current_frame = physical_address / PAGE_SIZE;
        void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

        size_t bytes_to_copy;
        if (i == 0) { //Primera pagina
            bytes_to_copy = PAGE_SIZE - (physical_address % PAGE_SIZE);
            bytes_to_copy = (bytes_to_copy > bytes) ? bytes : bytes_to_copy;
        } else if (i == size - 1) { //Ultima pagina
            bytes_to_copy = bytes;
        } else { //Paginas del medio (no ultima)
            bytes_to_copy = PAGE_SIZE;
        }

        pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
            payload_add(&(package->payload), posicion, bytes_to_copy);
            memcpy(text_to_send + offset, posicion, bytes_to_copy);
            update_page(current_frame);// Actualizar la página
        pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);

        offset += bytes_to_copy;
        bytes -= bytes_to_copy;

        log_debug(MODULE_LOGGER, "PID: <%" PRIu16 "> - Accion: <LEER> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes_to_copy);
    }

    // Le agrego un '\0' al final por las dudas para asegurar de que el string se pueda imprimir
    text_to_send[offset] = '\0';
    log_error(MODULE_LOGGER, "Texto a enviar: %s", text_to_send);

    package_send(package, socket);
    package_destroy(package);
    list_destroy(list_physical_addresses);
}

void copy_memory(t_Payload *payload, int socket) {
    t_PID pid;
    t_list *list_physical_addresses_origin = list_create();
    t_list *list_physical_addresses_destiny = list_create();
    size_t bytes_received;

    payload_remove(payload, &pid, sizeof(pid));
    list_deserialize(payload, list_physical_addresses_origin, size_deserialize_element);
    list_deserialize(payload, list_physical_addresses_destiny, size_deserialize_element);
    size_deserialize(payload, &bytes_received);

    size_t bytes = bytes_received;
    char text_to_send[bytes_received +1]; // Le agrego un '\0' al final por las dudas para asegurar de que el string se pueda imprimir
    size_t offset = 0;

    size_t physical_address = *((size_t *) list_get(list_physical_addresses_origin, 0));
    
    log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Accion: <LEER> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes);

    int size = list_size(list_physical_addresses_origin);
    for (int i = 0; i < size; i++) {
        physical_address = *((size_t *) list_get(list_physical_addresses_origin, i));
        size_t current_frame = physical_address / PAGE_SIZE;
        void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

        size_t bytes_to_copy;
        if (i == 0) { //Primera pagina
            bytes_to_copy = PAGE_SIZE - (physical_address % PAGE_SIZE);
            bytes_to_copy = (bytes_to_copy > bytes) ? bytes : bytes_to_copy;
        } else if (i == size - 1) { //Ultima pagina
            bytes_to_copy = bytes;
        } else { //Paginas del medio (no ultima)
            bytes_to_copy = PAGE_SIZE;
        }

        pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
            //payload_add(&(package->payload), posicion, bytes_to_copy);
            memcpy(text_to_send + offset, posicion, bytes_to_copy);
            update_page(current_frame);// Actualizar la página
        pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);

        offset += bytes_to_copy;
        bytes -= bytes_to_copy;

        log_debug(MODULE_LOGGER, "PID: <%" PRIu16 "> - Accion: <LEER> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes_to_copy);
    }

    text_to_send[offset] = '\0';
    log_error(MODULE_LOGGER, "Texto a copiar: %s", text_to_send);
    
    bytes = bytes_received;

    physical_address = *((size_t *) list_get(list_physical_addresses_destiny, 0));
    offset = 0;
    size = list_size(list_physical_addresses_destiny);
    log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Accion: <ESCRIBIR> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes);
        
        for (int i = 0; i < size; i++) {
            physical_address = *((size_t *) list_get(list_physical_addresses_destiny, i));
            size_t current_frame = physical_address / PAGE_SIZE;
            void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

            size_t bytes_to_copy;
            if (i == 0) { //Primera pagina
                bytes_to_copy = PAGE_SIZE - (physical_address % PAGE_SIZE);
                bytes_to_copy = (bytes_to_copy > bytes) ? bytes : bytes_to_copy;
            } else if (i == size - 1) { //Ultima pagina
                bytes_to_copy = bytes;
            } else { //Paginas del medio (no ultima)
                bytes_to_copy = PAGE_SIZE;
            }

            pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                //payload_remove(payload, posicion, bytes_to_copy);
                memcpy(posicion,text_to_send + offset, bytes_to_copy);
                update_page(current_frame);// Actualizar la página
            pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);

            offset += bytes_to_copy;
            bytes -= bytes_to_copy;

            log_debug(MODULE_LOGGER, "PID: <%" PRIu16 "> - Accion: <ESCRIBIR> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes_to_copy);
        }
    
    list_destroy_and_destroy_elements(list_physical_addresses_destiny, free);
    list_destroy_and_destroy_elements(list_physical_addresses_origin, free);

    if(send_return_value_with_header(COPY_REQUEST_HEADER, 0, socket)) {
        log_info(MODULE_LOGGER, "PID: %i - Accion: COPY FAIL!.", pid);
        exit(1);
    }

}

void io_write_memory(t_Payload *payload, int socket) {
    t_PID pid;
    t_list *list_physical_addresses = list_create();
    size_t bytes;
    
    payload_remove(payload, &pid, sizeof(pid));
    list_deserialize(payload, list_physical_addresses, size_deserialize_element);
    size_deserialize(payload, &bytes);

    size_t physical_address = *((size_t *) list_get(list_physical_addresses, 0));

    log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Accion: <ESCRIBIR> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes);

    //char text_to_send[bytes];
    size_t offset = 0;

//COMIENZA LA ESCRITURA
    int size = list_size(list_physical_addresses);

        for (int i = 0; i < size; i++) {
            physical_address = *((size_t *) list_get(list_physical_addresses, i));
            size_t current_frame = physical_address / PAGE_SIZE;
            void *posicion = (void *)(((uint8_t *) MAIN_MEMORY) + physical_address);

            size_t bytes_to_copy;
            if (i == 0) { //Primera pagina
                bytes_to_copy = PAGE_SIZE - (physical_address % PAGE_SIZE);
                bytes_to_copy = (bytes_to_copy > bytes) ? bytes : bytes_to_copy;
            } else if (i == size - 1) { //Ultima pagina
                bytes_to_copy = bytes;
            } else { //Paginas del medio (no ultima)
                bytes_to_copy = PAGE_SIZE;
            }

            char leido[bytes_to_copy+1];

            pthread_mutex_lock(&MUTEX_MAIN_MEMORY);
                payload_remove(payload, posicion, bytes_to_copy);
                //memcpy(text_to_send + offset, posicion, bytes_to_copy);
                memcpy(leido, posicion, bytes_to_copy);
                update_page(current_frame);// Actualizar la página
            pthread_mutex_unlock(&MUTEX_MAIN_MEMORY);
    leido[bytes_to_copy] = '\0';

            offset += bytes_to_copy;
            bytes -= bytes_to_copy;



            log_debug(MODULE_LOGGER, "PID: <%" PRIu16 "> - Accion: <ESCRIBIR> - Direccion fisica: <%zd> - Tamaño <%zd>", pid, physical_address, bytes_to_copy);
            log_error(MODULE_LOGGER, "PID: <%" PRIu16 "> - Accion: <ESCRIBIR> - TEXTO: %s", pid, leido);

        }
    
    list_destroy_and_destroy_elements(list_physical_addresses, free);

    if(send_return_value_with_header(WRITE_REQUEST_HEADER, 0, socket)) {
        // TODO
        exit(1);
    }
}

void read_memory(t_Payload *payload, int socket) {
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
}

void write_memory(t_Payload *payload, int socket) {
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
}

//Actualizar page y TDP
void update_page(size_t frame_number){
    t_Frame *frame = list_get(LIST_FRAMES, (int) frame_number);
    t_Page *current_page = frame->assigned_page;
    current_page->last_use = time(NULL);
}

//En caso de varias paginas
int get_next_dir_fis(size_t frame_number, t_PID pid){
    t_Frame *frame = (t_Frame *) list_get(LIST_FRAMES, frame_number);
    t_Page *current_page = frame->assigned_page;
    size_t page_number = current_page->page_number;
    t_Process *proceso = list_find_by_condition_with_comparation(LIST_PROCESSES, (bool (*)(void *, void *)) process_matches_pid, (void *) &pid);
    current_page = list_get(proceso->pages_table, (int) (page_number + 1));
    size_t next_frame_number = current_page->assigned_frame_number;
    size_t offset = 0;
    size_t next_dir_fis = next_frame_number * PAGE_SIZE + offset;

    return next_dir_fis;
}

void resize_process(t_Payload *payload){
    t_PID pid;
    size_t new_size;

    payload_remove(payload, &pid, sizeof(pid));
    size_deserialize(payload, &new_size);

    t_Process *process = list_find_by_condition_with_comparation(LIST_PROCESSES, (bool (*)(void *, void *)) process_matches_pid, (void *) &pid);

    size_t page_quantity = new_size / PAGE_SIZE;
    size_t remainder = new_size % PAGE_SIZE;

    if (remainder != 0)
        page_quantity += 1;

    int size = list_size(process->pages_table);
    t_Return_Value return_value;

    if(new_size > MEMORY_SIZE)
        return_value = 1;
    else {

        if(size < page_quantity) { //Agregar paginas

            //CASO: OUT OF MEMORY
            if(list_size(LIST_FREE_FRAMES) < (page_quantity - size))
                return_value = 1;
            else {
                
                log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Tamaño Actual: <%d> - Tamaño a Ampliar: <%zd>", pid, size, page_quantity);

                //CASO: HAY ESPACIO Y SUMA PAGINAS
                for (size_t i = size; i < page_quantity; i++)
                {
                    t_Page *page = malloc(sizeof(t_Page));
                    pthread_mutex_lock(&(MUTEX_LIST_FREE_FRAMES));
                        t_Frame *free_frame = list_get(LIST_FREE_FRAMES,0);
                        list_remove(LIST_FREE_FRAMES, 0);
                    pthread_mutex_unlock(&(MUTEX_LIST_FREE_FRAMES));
                    page->assigned_frame_number = free_frame->frame_number;
                    page->bit_modificado = false;
                    page->bit_presencia = false;
                    page->bit_uso = false;
                    page->page_number = i;
                    page->last_use = 0;

                    //Actualizo el marco asignado
                    free_frame->PID= pid;
                    free_frame->assigned_page = page;

                    list_add(process->pages_table, page);
                }
                    
                return_value = 0;
            }
        }

        if(size > page_quantity) { // RESTA paginas
                
            log_info(MINIMAL_LOGGER, "PID: <%" PRIu16 "> - Tamaño Actual: <%" PRIu32 "> - Tamaño a Reducir: <%zd>", pid, size, page_quantity);
            
            for(size_t i = size; i > page_quantity; i--) {
                int pos_lista = seek_oldest_page_updated(process->pages_table);
                t_Page *page = list_get(process->pages_table, pos_lista);
                list_remove(process->pages_table, pos_lista);
                pthread_mutex_lock(&(MUTEX_LIST_FREE_FRAMES));
                    t_Frame *frame = list_get(LIST_FRAMES, page->assigned_frame_number);
                    list_add(LIST_FREE_FRAMES, frame);
                pthread_mutex_unlock(&(MUTEX_LIST_FREE_FRAMES));

                free(page);
            }

            return_value = 0;
        }
    }
    //No hace falta el caso page == size ya que no sucederia nada

    if(send_return_value_with_header(RESIZE_REQUEST_HEADER, return_value, CLIENT_CPU->fd_client)) {
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

        log_debug(MODULE_LOGGER, "PID: <%" PRIu16 "> - Tamaño Actual: <%" PRIu32 "> - Tamaño a Reducir: <%zd>", pid, size, page_quantity);

        exit(EXIT_FAILURE);
    }
}

int seek_oldest_page_updated(t_list* page_list){

    int size = list_size(page_list);
    return (size - 1);
    
    /*
    t_Page* mas_antigua = list_get(page_list,0);
    
    int oldest_pos = 0;

    for (size_t i = 1; i < size; i++) {
        t_Page* page_temp = list_get(page_list,i);

        if (difftime(page_temp->last_use, mas_antigua->last_use) < 0) {
            mas_antigua = page_temp;
            oldest_pos = i;
        }
    }
    return oldest_pos;
    */
}

void free_memory(void){
    free_all_process();
    free_frames();
    free(MAIN_MEMORY);
}

void free_all_process(void){
    
    int size = list_size(LIST_PROCESSES);
    int size_TDP = 0;
    t_Process *processKilled;
    t_Page *pageKill;

    for (int i = (size -1); size > -1 ; size--)
    {
        processKilled = list_get(LIST_PROCESSES,i);
        size_TDP = list_size(processKilled->pages_table);
        for (int p = (size_TDP -1); size_TDP > -1 ; p--)
        {
            pageKill = list_get(processKilled->pages_table, p);
            free(pageKill);
        }
        
        free(processKilled);
    }
    
    log_debug(MODULE_LOGGER, "Se ha liberado todos los procesos y paginas.");
    
}