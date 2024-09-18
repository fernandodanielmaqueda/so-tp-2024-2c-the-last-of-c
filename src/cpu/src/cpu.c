/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "cpu.h"

char *MODULE_NAME = "cpu";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "cpu.log";

t_config *MODULE_CONFIG;
char *MODULE_CONFIG_PATHNAME = "cpu.config";

t_PID PID;
t_TID TID;
t_Exec_Context EXEC_CONTEXT;
size_t BASE;
size_t LIMIT;
pthread_mutex_t MUTEX_EXEC_CONTEXT;

bool EXECUTING = 0;
pthread_mutex_t MUTEX_EXECUTING;

e_Eviction_Reason EVICTION_REASON;

e_Kernel_Interrupt KERNEL_INTERRUPT;
pthread_mutex_t MUTEX_KERNEL_INTERRUPT;

bool SYSCALL_CALLED;
t_Payload SYSCALL_INSTRUCTION;

int module(int argc, char *argv[])
{

    if(initialize_configs(MODULE_CONFIG_PATHNAME)) {
        // TODO
        exit(EXIT_FAILURE);
    }

    initialize_loggers();
    initialize_global_variables();
    initialize_sockets();

    pthread_create(&(CLIENT_KERNEL_CPU_INTERRUPT.thread_client_handler), NULL, (void *(*)(void *)) kernel_cpu_interrupt_handler, NULL);

    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);
    instruction_cycle();

    // finish_threads();
    finish_sockets();
    // finish_configs();
    finish_loggers();
	finish_global_variables();

    return EXIT_SUCCESS;
}

int initialize_global_variables(void) {
    pthread_mutex_init(&MUTEX_EXEC_CONTEXT, NULL);
    pthread_mutex_init(&MUTEX_EXECUTING, NULL);
    pthread_mutex_init(&MUTEX_KERNEL_INTERRUPT, NULL);

    return 0;
}

int finish_global_variables(void) {
    pthread_mutex_destroy(&MUTEX_EXEC_CONTEXT);
    pthread_mutex_destroy(&MUTEX_EXECUTING);
    pthread_mutex_destroy(&MUTEX_KERNEL_INTERRUPT);

    return 0;
}

int read_module_config(t_config *MODULE_CONFIG) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "PUERTO_ESCUCHA_DISPATCH", "PUERTO_ESCUCHA_INTERRUPT", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

    CONNECTION_MEMORY = (t_Connection){.client_type = CPU_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};

    SERVER_CPU_DISPATCH = (t_Server){.server_type = CPU_DISPATCH_PORT_TYPE, .clients_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_DISPATCH")};
    CLIENT_KERNEL_CPU_DISPATCH = (t_Client){.client_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .server = &SERVER_CPU_DISPATCH};

    SERVER_CPU_INTERRUPT = (t_Server){.server_type = CPU_INTERRUPT_PORT_TYPE, .clients_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_INTERRUPT")};
    CLIENT_KERNEL_CPU_INTERRUPT = (t_Client){.client_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .server = &SERVER_CPU_INTERRUPT};

    LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));

    return 0;
}

void instruction_cycle(void)
{

    char *IR;
    t_Arguments *arguments = arguments_create(MAX_CPU_INSTRUCTION_ARGUMENTS);
    e_CPU_OpCode cpu_opcode;
    int status;

    while(1) {

        pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT);
            KERNEL_INTERRUPT = NONE_KERNEL_INTERRUPT;
        pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);

        payload_init(&SYSCALL_INSTRUCTION);

        pthread_mutex_lock(&MUTEX_EXEC_CONTEXT);
            if(receive_pid_and_tid_with_expected_header(THREAD_DISPATCH_HEADER, &PID, &TID, CLIENT_KERNEL_CPU_DISPATCH.fd_client)) {
                // TODO
                exit(EXIT_FAILURE);
            }

            // Esto funciona como solicitud a memoria para que me mande el contexto de ejecución
            if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, PID, TID, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(EXIT_FAILURE);
            }

            log_info(MINIMAL_LOGGER, "## TID: %u - Solicito Contexto Ejecución", TID);

            // Recibo la respuesta de memoria con el contexto de ejecución
            if(receive_exec_context(&EXEC_CONTEXT, &BASE, &LIMIT, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(EXIT_FAILURE);
            }
        pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT);

        pthread_mutex_lock(&MUTEX_EXECUTING);
            EXECUTING = 1;
        pthread_mutex_unlock(&MUTEX_EXECUTING);

        log_trace(MODULE_LOGGER, "<%d:%d> Contexto de ejecucion recibido del proceso - Ciclo de instruccion ejecutando", PID, TID);

        while(1) {

            // Fetch
            log_info(MINIMAL_LOGGER, "## TID: %u - FETCH - Program Counter: %u", TID, EXEC_CONTEXT.PC);
            cpu_fetch_next_instruction(&IR);
            if(IR == NULL) {
                log_error(MODULE_LOGGER, "Error al fetchear la instruccion");
                EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                break;
            }

            // Decode
            status = arguments_use(arguments, IR);
            if(status) {
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
            status = CPU_OPERATIONS[cpu_opcode].function(arguments->argc, arguments->argv);

            arguments_remove(arguments);
            free(IR);

            if(status) {
                log_trace(MODULE_LOGGER, "Error en la ejecucion de la instruccion");
                // EVICTION_REASON ya debe ser asignado por la instrucción cuando falla
                break;
            }

            // Check Interrupts
            if((cpu_opcode == PROCESS_EXIT_CPU_OPCODE) || (cpu_opcode == THREAD_EXIT_CPU_OPCODE)) {
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
            log_info(MINIMAL_LOGGER, "## TID: %u - Actualizo Contexto Ejecución", TID);

            if(send_exec_context_update(PID, TID, EXEC_CONTEXT, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(EXIT_FAILURE);
            }

            if(send_thread_eviction(EVICTION_REASON, SYSCALL_INSTRUCTION, CLIENT_KERNEL_CPU_DISPATCH.fd_client)) {
                // TODO
                exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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

        log_info(MINIMAL_LOGGER, "## Llega interrupción al puerto Interrupt");

        pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT);
            // Una forma de establecer prioridad entre interrupciones que se pisan, sólo va a quedar una
            if (KERNEL_INTERRUPT < kernel_interrupt)
                KERNEL_INTERRUPT = kernel_interrupt;
        pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT);

    }

    return NULL;
}

int cpu_fetch_next_instruction(char **line) {
    if(line == NULL) {
        errno = EINVAL;
        return -1;
    }

    if(send_instruction_request(PID, TID, EXEC_CONTEXT.PC, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        return -1;
    }
    if(receive_text_with_expected_header(INSTRUCTION_REQUEST_HEADER, line, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        return -1;
    }

    return 0;
}

int mmu(size_t logical_address, size_t bytes, size_t *destination) {
    // Verifico que el puntero de destino no sea nulo
    if(destination == NULL) {
        errno = EINVAL;
        return -1;
    }

    // Verifico que no produzca overflow
    if(logical_address > (SIZE_MAX - BASE) || bytes > (SIZE_MAX - (BASE + logical_address))) {
        errno = ERANGE;
        return -1;
    }

    // Calculo la dirección física
    size_t physical_address = BASE + logical_address;
    // La dirección lógica es el desplazamiento desde la base

    // Verifico que no haya segmentation fault
    if((physical_address + bytes) >= LIMIT) {
        errno = EFAULT;
        return -1;
    }

    // Asigno la dirección física al puntero de destino
    *destination = physical_address;

    return 0;
}

int write_memory(size_t physical_address, void *source, size_t bytes) {
    if(source == NULL) {
        errno = EINVAL;
        return -1;
    }

    if(send_write_request(PID, TID, physical_address, source, bytes, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        return -1;
    }

    if(receive_expected_header(WRITE_REQUEST_HEADER, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        return -1;
    }

    return 0;
}

int read_memory(size_t physical_address, void *destination, size_t bytes) {
    if(destination == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    t_Package* package;

    if(send_read_request(PID, TID, physical_address, bytes, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(EXIT_FAILURE);
    }

    if(package_receive(&package, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(EXIT_FAILURE);
    }
    payload_remove(&(package->payload), destination, bytes);
    package_destroy(package);

    return 0;
}