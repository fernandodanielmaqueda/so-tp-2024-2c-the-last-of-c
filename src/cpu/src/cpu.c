/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "cpu.h"

char *MODULE_NAME = "cpu";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "cpu.log";

char *MODULE_CONFIG_PATHNAME = "cpu.config";
t_config *MODULE_CONFIG;

int TLB_ENTRY_COUNT;

const char *TLB_ALGORITHMS[] = {
    [FIFO_TLB_ALGORITHM] = "FIFO",
    [LRU_TLB_ALGORITHM] = "LRU"
};

e_TLB_Algorithm TLB_ALGORITHM;

size_t PAGE_SIZE;
long TIMESTAMP;
t_list *TLB;          // TLB que voy a ir creando para darle valores que obtengo de la estructura de t_tlb

t_Exec_Context EXEC_CONTEXT;
pthread_mutex_t MUTEX_EXEC_CONTEXT;

int EXECUTING = 0;
pthread_mutex_t MUTEX_EXECUTING;

e_Eviction_Reason EVICTION_REASON;

e_Kernel_Interrupt KERNEL_INTERRUPT;
pthread_mutex_t MUTEX_KERNEL_INTERRUPT;

int SYSCALL_CALLED;
t_Payload SYSCALL_INSTRUCTION;

int TLB_REPLACE_INDEX_FIFO = 0;

pthread_mutex_t MUTEX_TLB;

int module(int argc, char *argv[])
{

    initialize_loggers();
    initialize_configs(MODULE_CONFIG_PATHNAME);
    initialize_mutexes();
	initialize_semaphores();
    initialize_sockets();

    pthread_create(&(CLIENT_KERNEL_CPU_INTERRUPT.thread_client_handler), NULL, (void *(*)(void *)) kernel_cpu_interrupt_handler, NULL);

    TLB = list_create();

    //Se pide a memoria el tamaño de pagina y lo setea como dato global
    ask_memory_page_size();

    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);
    instruction_cycle();

    // finish_threads();
    finish_sockets();
    // finish_configs();
    finish_loggers();
	finish_semaphores();
	finish_mutexes();

    return EXIT_SUCCESS;
}

void initialize_mutexes(void) {
    pthread_mutex_init(&MUTEX_EXEC_CONTEXT, NULL);
    pthread_mutex_init(&MUTEX_EXECUTING, NULL);
    pthread_mutex_init(&MUTEX_KERNEL_INTERRUPT, NULL);    
    pthread_mutex_init(&MUTEX_TLB, NULL);
}

void finish_mutexes(void) {
    pthread_mutex_destroy(&MUTEX_EXEC_CONTEXT);
    pthread_mutex_destroy(&MUTEX_EXECUTING);
    pthread_mutex_destroy(&MUTEX_KERNEL_INTERRUPT);    
    pthread_mutex_destroy(&MUTEX_TLB);
}

void initialize_semaphores(void) {
    
}

void finish_semaphores(void) {
    
}

void read_module_config(t_config *MODULE_CONFIG)
{
    CONNECTION_MEMORY = (t_Connection){.client_type = CPU_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};
    
    SERVER_CPU_DISPATCH = (t_Server){.server_type = CPU_DISPATCH_PORT_TYPE, .clients_type = KERNEL_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_DISPATCH")};
    CLIENT_KERNEL_CPU_DISPATCH = (t_Client){.client_type = KERNEL_PORT_TYPE, .server = &SERVER_CPU_DISPATCH};
    
    SERVER_CPU_INTERRUPT = (t_Server){.server_type = CPU_INTERRUPT_PORT_TYPE, .clients_type = KERNEL_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_INTERRUPT")};
    CLIENT_KERNEL_CPU_INTERRUPT = (t_Client){.client_type = KERNEL_PORT_TYPE, .server = &SERVER_CPU_INTERRUPT};
    
    TLB_ENTRY_COUNT = config_get_int_value(MODULE_CONFIG, "CANTIDAD_ENTRADAS_TLB");

    if(find_tlb_algorithm(config_get_string_value(MODULE_CONFIG, "ALGORITMO_TLB"), &TLB_ALGORITHM)) {
		log_error(MODULE_LOGGER, "ALGORITMO_PLANIFICACION invalido");
		exit(EXIT_FAILURE);
	}
}

int find_tlb_algorithm(char *name, e_TLB_Algorithm *destination) {

    if(name == NULL || destination == NULL)
        return 1;
    
    size_t tlb_algorithms_number = sizeof(TLB_ALGORITHMS) / sizeof(TLB_ALGORITHMS[0]);
    for (register e_TLB_Algorithm tlb_algorithm = 0; tlb_algorithm < tlb_algorithms_number; tlb_algorithm++)
        if (strcmp(TLB_ALGORITHMS[tlb_algorithm], name) == 0) {
            *destination = tlb_algorithm;
            return 0;
        }

    return 1;
}

void instruction_cycle(void)
{

    char *IR;
    t_Arguments *arguments = arguments_create(MAX_CPU_INSTRUCTION_ARGUMENTS);
    e_CPU_OpCode cpu_opcode;
    int exit_status;

    while(1) {

        pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT);
            KERNEL_INTERRUPT = NONE_KERNEL_INTERRUPT;
        pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);

        payload_init(&SYSCALL_INSTRUCTION);

        pthread_mutex_lock(&MUTEX_EXEC_CONTEXT);
            if(receive_process_dispatch(&EXEC_CONTEXT, CLIENT_KERNEL_CPU_DISPATCH.fd_client)) {
                // TODO
                exit(1);
            }
        pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT);

        pthread_mutex_lock(&MUTEX_EXECUTING);
            EXECUTING = 1;
        pthread_mutex_unlock(&MUTEX_EXECUTING);

        log_trace(MODULE_LOGGER, "Contexto de ejecucion recibido del proceso : %i - Ciclo de instruccion ejecutando", EXEC_CONTEXT.PID);

        while(1) {

            // Fetch
            log_debug(MINIMAL_LOGGER,"PID: %" PRIu16 " - FETCH - Program Counter: %" PRIu32, EXEC_CONTEXT.PID, EXEC_CONTEXT.PC);
            cpu_fetch_next_instruction(&IR);
            if(IR == NULL) {
                log_error(MODULE_LOGGER, "Error al fetchear la instruccion");
                EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                break;
            }

            // Decode
            exit_status = arguments_use(arguments, IR);
            if(exit_status) {
                switch(errno) {
                    case E2BIG:
                        log_error(MODULE_LOGGER, "%s: Demasiados argumentos en la instruccion", IR);
                        break;
                    case ENOMEM:
                        log_error(MODULE_LOGGER, "arguments_use: Error al reservar memoria para los argumentos");
                        exit(EXIT_FAILURE);
                    default:
                        log_error(MODULE_LOGGER, "arguments_use: %s", strerror(errno));
                        break;
                }
                arguments_remove(arguments);
                free(IR);
                EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                break;
            }

            if(decode_instruction(arguments->argv[0], &cpu_opcode)) {
                log_error(MODULE_LOGGER, "%s: Error al decodificar la instruccion", arguments->argv[0]);
                arguments_remove(arguments);
                free(IR);
                EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                break;
            }

            // Execute
            exit_status = CPU_OPERATIONS[cpu_opcode].function(arguments->argc, arguments->argv);

            arguments_remove(arguments);
            free(IR);

            if(exit_status) {
                log_trace(MODULE_LOGGER, "Error en la ejecucion de la instruccion");
                // EVICTION_REASON ya debe ser asignado por la instrucción cuando falla
                break;
            }

            // Check Interrupts
            if(cpu_opcode == EXIT_CPU_OPCODE) {
                EVICTION_REASON = EXIT_EVICTION_REASON;
                break;
            }

            pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT);
                if (KERNEL_INTERRUPT == KILL_KERNEL_INTERRUPT) {
                    pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);
                    EVICTION_REASON = KILL_KERNEL_INTERRUPT_EVICTION_REASON;
                    break;
                }
            pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);

            if(SYSCALL_CALLED) {
                EVICTION_REASON = SYSCALL_EVICTION_REASON;
                break;
            }

            pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT);
                if (KERNEL_INTERRUPT == QUANTUM_KERNEL_INTERRUPT) {
                    pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);
                    EVICTION_REASON = QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON;
                    break;
                }
            pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);
        }

        pthread_mutex_lock(&MUTEX_EXECUTING);
            EXECUTING = 0;
        pthread_mutex_unlock(&MUTEX_EXECUTING);

        pthread_mutex_lock(&MUTEX_EXEC_CONTEXT);
            if(send_process_eviction(EXEC_CONTEXT, EVICTION_REASON, SYSCALL_INSTRUCTION, CLIENT_KERNEL_CPU_DISPATCH.fd_client)) {
                // TODO
                exit(1);
            }
        pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT);

        payload_destroy(&SYSCALL_INSTRUCTION);
    }

    arguments_destroy(arguments);
}

void *kernel_cpu_interrupt_handler(void *NULL_parameter) {

    log_trace(MODULE_LOGGER, "Hilo de manejo de interrupciones de Kernel iniciado");

    e_Kernel_Interrupt kernel_interrupt;
    t_PID pid;

    while(1) {

        if(receive_kernel_interrupt(&kernel_interrupt, &pid, CLIENT_KERNEL_CPU_INTERRUPT.fd_client)) {
            // TODO
            exit(1);
        }

        pthread_mutex_lock(&MUTEX_EXECUTING);
            if(!EXECUTING) {
                pthread_mutex_unlock(&MUTEX_EXECUTING);
                continue;
            }
        pthread_mutex_unlock(&MUTEX_EXECUTING);

        pthread_mutex_lock(&MUTEX_EXEC_CONTEXT);
            if(pid != EXEC_CONTEXT.PID) {
                pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT);
                continue;
            }
        pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT);

        pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT);
            // Una forma de establecer prioridad entre interrupciones que se pisan, sólo va a quedar una
            if (KERNEL_INTERRUPT < kernel_interrupt)
                KERNEL_INTERRUPT = kernel_interrupt;
        pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);

    }

    return NULL;
}

t_list *mmu(t_PID pid, size_t logical_address, size_t bytes) {

    size_t page_number = (size_t) floor((double) logical_address / (double) PAGE_SIZE);
    size_t offset = logical_address - page_number * PAGE_SIZE;

    t_list *list_physical_addresses = list_create();
    size_t required_page_quantity = seek_quantity_pages_required(logical_address, bytes);

    size_t frame_number;
    size_t *physical_address;
    for(size_t i = 0; i < required_page_quantity; i++) {

        // CHEQUEO SI ESTA EN TLB EL FRAME QUE NECESITO
        pthread_mutex_lock(&MUTEX_TLB);
        if(check_tlb(pid, page_number, &frame_number)) { // NO HAY HIT
            pthread_mutex_unlock(&MUTEX_TLB);

            log_debug(MINIMAL_LOGGER, "PID: %" PRIu16 " - TLB MISS - PAGINA: %zd", pid, page_number);

            request_frame_memory(pid, page_number);

            t_Package *package;
            if(package_receive(&package, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(1);
            }
            t_Return_Value return_value;
            return_value_deserialize(&(package->payload), &return_value);
            if(return_value) {
                log_error(MODULE_LOGGER, "No se pudo obtener el número de marco correspondiente al número de pagina %zd de memoria", page_number);
                // TODO
                return NULL;
            }
            size_deserialize(&(package->payload), &frame_number);
            package_destroy(package);
            
            log_debug(MODULE_LOGGER, "PID: %" PRIu16 " - OBTENER MARCO - Página: %zd - Marco: %zd", pid, page_number, frame_number);

            if (TLB_ENTRY_COUNT > 0) {
                if (list_size(TLB) < TLB_ENTRY_COUNT)
                {
                    add_to_tlb(pid, page_number, frame_number);
                    log_trace(MODULE_LOGGER, "Agrego entrada a la TLB");
                }
                else
                {
                    replace_tlb_input(pid, page_number, frame_number);
                    log_trace(MODULE_LOGGER, "Reemplazo entrada a la TLB");
                }
            }

        }
        else { // HAY HIT
            pthread_mutex_unlock(&MUTEX_TLB);

            log_debug(MINIMAL_LOGGER, "PID: %" PRIu16 " - TLB HIT - PAGINA: %zd", pid, page_number);
            log_debug(MODULE_LOGGER, "PID: %" PRIu16 " - OBTENER MARCO - Página: %zd - Marco: %zd", pid, page_number, frame_number);

        }

        physical_address = malloc(sizeof(size_t));
        if(physical_address == NULL) {
            log_error(MODULE_LOGGER, "No se pudo reservar memoria para la direccion fisica");
            exit(EXIT_FAILURE);
        }

        *physical_address = frame_number * PAGE_SIZE + offset;

        log_debug(MODULE_LOGGER, "PID: %" PRIu16 " - PAGINA: %zd - DF: %zd", pid, page_number, *physical_address); // CAMBIARLO A MODULE_LOGGER
        if(offset)
            offset = 0; //El offset solo es importante en la 1ra pagina buscada

        list_add(list_physical_addresses, physical_address);
        page_number ++;
    }
    
    return list_physical_addresses;
}

int check_tlb(t_PID process_id, size_t page_number, size_t *destination_frame_number) {

    t_TLB *tlb_entry = NULL;
    for (int i = 0; i < list_size(TLB); i++) {

        tlb_entry = (t_TLB *) list_get(TLB, i);
        if (tlb_entry->PID == process_id && tlb_entry->page_number == page_number) {
            *destination_frame_number = tlb_entry->frame_number;

            if(TLB_ALGORITHM == LRU_TLB_ALGORITHM) {
                tlb_entry->time = TIMESTAMP;
                TIMESTAMP++;
            }

            return 0;
        }
    }

    return 1;
}

void add_to_tlb(t_PID pid , size_t page_number, size_t frame_number) {
    t_TLB *tlb_entry = malloc(sizeof(t_TLB));
    tlb_entry->PID = pid;
    tlb_entry->page_number = page_number;
    tlb_entry->frame_number = frame_number;
    tlb_entry->time = TIMESTAMP;
    TIMESTAMP++;
    list_add(TLB, tlb_entry);
}

void delete_tlb_entry_by_pid_on_resizing(t_PID pid, int resize_number) {
    t_TLB *tlb_entry;
    int size = list_size(TLB);

    if(size != 0){

        for (size_t i = (size -1); i != -1; i--)
        {
            tlb_entry = list_get(TLB, i);
            if((tlb_entry->PID == pid) && (tlb_entry->page_number >= (resize_number -1))){
                list_remove(TLB, i);
                free(tlb_entry);
            }
        }
    }
}

void delete_tlb_entry_by_pid_deleted(t_PID pid) {
    t_TLB *tlb_entry;
    int size = list_size(TLB);

    if(size != 0) {
        for (size_t i = (size -1); i != -1; i--) {
            tlb_entry = list_get(TLB, i);
            if(tlb_entry->PID == pid) {
                list_remove(TLB, i);
                free(tlb_entry);
            }
        }
    }
}

void replace_tlb_input(t_PID pid, size_t page_number, size_t frame_number) {
    t_TLB *tlb_aux;

    switch(TLB_ALGORITHM) {
        case LRU_TLB_ALGORITHM:
        {
            tlb_aux = list_get(TLB, 0);
            int replace_value = tlb_aux->time + 1;
            int index_replace = 0;
                for(int i = 0; i < list_size(TLB); i++){

                    t_TLB *replaced_tlb = list_get(TLB, i);
                    if(replaced_tlb->time < replace_value){
                        replace_value = replaced_tlb->time;
                        index_replace = i;
                    } //guardo el d emenor tiempo
                }

            tlb_aux = list_get(TLB, index_replace);
            tlb_aux->PID = pid;
            tlb_aux->page_number = page_number;
            tlb_aux->frame_number = frame_number;
            tlb_aux->time = TIMESTAMP;
            TIMESTAMP++;
            break;
        }
        case FIFO_TLB_ALGORITHM:
        {
            tlb_aux = list_get(TLB, TLB_REPLACE_INDEX_FIFO);
            tlb_aux->PID = pid;
            tlb_aux->page_number = page_number;
            tlb_aux->frame_number = frame_number;
            tlb_aux->time = TIMESTAMP;
            TIMESTAMP++;

            TLB_REPLACE_INDEX_FIFO++;
            if (TLB_REPLACE_INDEX_FIFO == list_size(TLB))
                TLB_REPLACE_INDEX_FIFO = 0;
            break;
        }
    }   
}

void request_frame_memory(t_PID pid, size_t page_number) {
    t_Package *package = package_create_with_header(FRAME_REQUEST);
    size_serialize(&(package->payload), page_number);
    payload_add(&(package->payload), &pid, sizeof(pid));
    package_send(package, CONNECTION_MEMORY.fd_connection);
}

void cpu_fetch_next_instruction(char **line) {
    if(send_instruction_request(EXEC_CONTEXT.PID, EXEC_CONTEXT.PC, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
    if(receive_text_with_expected_header(INSTRUCTION_REQUEST, line, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
}

void ask_memory_page_size(void) {
    if(send_header(PAGE_SIZE_REQUEST, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }

    t_Package* package;
    if(package_receive(&package, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
    size_deserialize(&(package->payload), &PAGE_SIZE);
    package_destroy(package);
}

void attend_write(t_PID pid, t_list *list_physical_addresses, char* source, size_t bytes) {

    t_Package* package;

    package = package_create_with_header(WRITE_REQUEST);
    payload_add(&(package->payload), &pid, sizeof(pid));
    list_serialize(&(package->payload), *list_physical_addresses, size_serialize_element);
    size_serialize(&(package->payload), bytes);
    payload_add(&(package->payload), source, (size_t) bytes);
    package_send(package, CONNECTION_MEMORY.fd_connection);
    package_destroy(package);

    if(receive_expected_header(WRITE_REQUEST,CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
    log_info(MODULE_LOGGER, "PID: %i - Accion: ESCRIBIR OK", pid);
}

void attend_read(t_PID pid, t_list *list_physical_addresses, void *destination, size_t bytes) {
    if(list_physical_addresses == NULL || destination == NULL)
        return;

    t_Package* package = package_create_with_header(READ_REQUEST);
    payload_add(&(package->payload), &(pid), sizeof(pid));
    list_serialize(&(package->payload), *list_physical_addresses, size_serialize_element);
    size_serialize(&(package->payload), bytes);          
    package_send(package, CONNECTION_MEMORY.fd_connection);
    package_destroy(package);

    

    if(package_receive(&package, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }

    log_info(MODULE_LOGGER, "DENTRO DE CPU FAIL 1");
    payload_remove(&(package->payload), &destination, bytes);
    log_info(MODULE_LOGGER, "DENTRO DE CPU FAIL 2222");
    package_destroy(package);
    log_info(MODULE_LOGGER, "DENTRO DE CPU FAIL 33333");
}

void attend_copy(t_PID pid, t_list *list_physical_addresses_origin, t_list *list_physical_addresses_destination, size_t bytes) {
    if(list_physical_addresses_origin == NULL || list_physical_addresses_destination == NULL)
        return;

    t_Package* package = package_create_with_header(COPY_REQUEST);
    payload_add(&(package->payload), &(pid), sizeof(pid));
    list_serialize(&(package->payload), *list_physical_addresses_origin, size_serialize_element);
    list_serialize(&(package->payload), *list_physical_addresses_destination, size_serialize_element);
    size_serialize(&(package->payload), bytes);          
    package_send(package, CONNECTION_MEMORY.fd_connection);
    package_destroy(package);

    
    if(receive_expected_header(COPY_REQUEST,CONNECTION_MEMORY.fd_connection)) {
        log_info(MODULE_LOGGER, "PID: %i - Accion: COPY FAIL!.", pid);
        exit(1);
    }
    log_info(MODULE_LOGGER, "PID: %i - Accion: COPY OK", pid);

}

size_t seek_quantity_pages_required(size_t logical_address, size_t bytes){
    size_t page_quantity = 0;
    
    size_t offset = logical_address % PAGE_SIZE;

    if (offset != 0) {
        // Calcula los bytes restantes en la primera página
        size_t bytes_in_first_page = PAGE_SIZE - offset;

        if(bytes <= bytes_in_first_page)
            return 1;
        else {
            bytes -= bytes_in_first_page;
            page_quantity++;
        }
    }

    page_quantity += (size_t) ceil((double) bytes / (double) PAGE_SIZE);
    
    return page_quantity;
}