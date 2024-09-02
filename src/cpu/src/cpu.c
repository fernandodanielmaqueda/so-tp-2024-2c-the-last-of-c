/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "cpu.h"

char *MODULE_NAME = "cpu";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "cpu.log";

char *MODULE_CONFIG_PATHNAME = "cpu.config";
t_config *MODULE_CONFIG;

t_PID PID;
t_TID TID;
t_Exec_Context EXEC_CONTEXT;
pthread_mutex_t MUTEX_EXEC_CONTEXT;

int EXECUTING = 0;
pthread_mutex_t MUTEX_EXECUTING;

e_Eviction_Reason EVICTION_REASON;

e_Kernel_Interrupt KERNEL_INTERRUPT;
pthread_mutex_t MUTEX_KERNEL_INTERRUPT;

int SYSCALL_CALLED;
t_Payload SYSCALL_INSTRUCTION;

int module(int argc, char *argv[])
{

    initialize_configs(MODULE_CONFIG_PATHNAME);
    initialize_loggers();
    initialize_mutexes();
	initialize_semaphores();
    initialize_sockets();

    pthread_create(&(CLIENT_KERNEL_CPU_INTERRUPT.thread_client_handler), NULL, (void *(*)(void *)) kernel_cpu_interrupt_handler, NULL);

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
}

void finish_mutexes(void) {
    pthread_mutex_destroy(&MUTEX_EXEC_CONTEXT);
    pthread_mutex_destroy(&MUTEX_EXECUTING);
    pthread_mutex_destroy(&MUTEX_KERNEL_INTERRUPT);
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

    LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));
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
            if(receive_pid_and_tid_with_expected_header(THREAD_DISPATCH_HEADER, &PID, &TID, CLIENT_KERNEL_CPU_DISPATCH.fd_client)) {
                // TODO
                exit(1);
            }

            if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, PID, TID, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(1);
            }

            if(receive_exec_context(&EXEC_CONTEXT, CONNECTION_MEMORY.fd_connection)) {
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
            log_info(MINIMAL_LOGGER,"PID: %" PRIu16 " - FETCH - Program Counter: %" PRIu32, EXEC_CONTEXT.PID, EXEC_CONTEXT.PC);
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
            if(send_exec_context_update(PID, TID, EXEC_CONTEXT, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(1);
            }

            if(send_thread_eviction(EVICTION_REASON, SYSCALL_INSTRUCTION, CLIENT_KERNEL_CPU_DISPATCH.fd_client)) {
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
    t_TID tid;

    while(1) {

        if(receive_kernel_interrupt(&kernel_interrupt, &pid, &tid, CLIENT_KERNEL_CPU_INTERRUPT.fd_client)) {
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
            if(pid != PID && tid != TID) {
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

    /*
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

            log_info(MINIMAL_LOGGER, "PID: %" PRIu16 " - TLB MISS - PAGINA: %zd", pid, page_number);

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

            log_info(MINIMAL_LOGGER, "PID: %" PRIu16 " - TLB HIT - PAGINA: %zd", pid, page_number);
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
    */
   return NULL;
}

void cpu_fetch_next_instruction(char **line) {
    if(send_instruction_request(PID, TID, EXEC_CONTEXT.PC, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
    if(receive_text_with_expected_header(INSTRUCTION_REQUEST_HEADER, line, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
}

/*
void request_frame_memory(t_PID pid, size_t page_number) {
    t_Package *package = package_create_with_header(FRAME_REQUEST_HEADER);
    size_serialize(&(package->payload), page_number);
    payload_add(&(package->payload), &pid, sizeof(pid));
    package_send(package, CONNECTION_MEMORY.fd_connection);
}
*/

/*
void attend_write(t_PID pid, t_list *list_physical_addresses, char* source, size_t bytes) {

    t_Package* package;

    package = package_create_with_header(WRITE_REQUEST_HEADER);
    payload_add(&(package->payload), &pid, sizeof(pid));
    list_serialize(&(package->payload), *list_physical_addresses, size_serialize_element);
    size_serialize(&(package->payload), bytes);
    payload_add(&(package->payload), source, (size_t) bytes);
    package_send(package, CONNECTION_MEMORY.fd_connection);
    package_destroy(package);

    if(receive_expected_header(WRITE_REQUEST_HEADER ,CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
    log_info(MODULE_LOGGER, "PID: %i - Accion: ESCRIBIR OK", pid);
}

void attend_read(t_PID pid, t_list *list_physical_addresses, void *destination, size_t bytes) {
    if(list_physical_addresses == NULL || destination == NULL)
        return;

    t_Package* package = package_create_with_header(READ_REQUEST_HEADER);
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

    t_Package* package = package_create_with_header(COPY_REQUEST_HEADER);
    payload_add(&(package->payload), &(pid), sizeof(pid));
    list_serialize(&(package->payload), *list_physical_addresses_origin, size_serialize_element);
    list_serialize(&(package->payload), *list_physical_addresses_destination, size_serialize_element);
    size_serialize(&(package->payload), bytes);          
    package_send(package, CONNECTION_MEMORY.fd_connection);
    package_destroy(package);

    
    if(receive_expected_header(COPY_REQUEST_HEADER ,CONNECTION_MEMORY.fd_connection)) {
        log_info(MODULE_LOGGER, "PID: %i - Accion: COPY FAIL!.", pid);
        exit(1);
    }
    log_info(MODULE_LOGGER, "PID: %i - Accion: COPY OK", pid);

}
*/