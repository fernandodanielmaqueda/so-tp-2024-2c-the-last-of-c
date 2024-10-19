/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "cpu.h"

char *MODULE_NAME = "cpu";

t_log *MODULE_LOGGER = NULL;
char *MODULE_LOG_PATHNAME = "cpu.log";

t_config *MODULE_CONFIG = NULL;
char *MODULE_CONFIG_PATHNAME = "cpu.config";

t_PID PID;
t_TID TID;
t_Exec_Context EXEC_CONTEXT;
pthread_mutex_t MUTEX_EXEC_CONTEXT;
bool IS_MUTEX_EXEC_CONTEXT_INITIALIZED = false;

bool EXECUTING = 0;
pthread_mutex_t MUTEX_EXECUTING;
bool IS_MUTEX_EXECUTING_INITIALIZED = false;

e_Eviction_Reason EVICTION_REASON;

e_Kernel_Interrupt KERNEL_INTERRUPT;
pthread_mutex_t MUTEX_KERNEL_INTERRUPT;
bool IS_MUTEX_KERNEL_INTERRUPT_INITIALIZED = false;

bool SYSCALL_CALLED;
t_Payload SYSCALL_INSTRUCTION = {0};

int module(int argc, char *argv[])
{
    int status;

    if(initialize_configs(MODULE_CONFIG_PATHNAME)) {
        // TODO
        goto error;
    }

    initialize_loggers();
    initialize_global_variables();
    initialize_sockets();

    if((status = pthread_create(&(CLIENT_KERNEL_CPU_INTERRUPT.thread_client_handler), NULL, (void *(*)(void *)) kernel_cpu_interrupt_handler, NULL))) {
        log_error_pthread_create(status);
        // TODO
    }

    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);
    instruction_cycle();

    // finish_threads();
    finish_sockets();
    // finish_configs();
    finish_loggers();
	finish_global_variables();

    return EXIT_SUCCESS;

    error:
        return EXIT_FAILURE;
}

int initialize_global_variables(void) {
    int status;

    if((status = pthread_mutex_init(&MUTEX_EXEC_CONTEXT, NULL))) {
        log_error_pthread_mutex_init(status);
        goto error;
    }

    if((status = pthread_mutex_init(&MUTEX_EXECUTING, NULL))) {
        log_error_pthread_mutex_init(status);
        goto error_mutex_exec_context;
    }

    if((status = pthread_mutex_init(&MUTEX_KERNEL_INTERRUPT, NULL))) {
        log_error_pthread_mutex_init(status);
        goto error_mutex_executing;
    }

    return 0;

    error_mutex_executing:
        if((status = pthread_mutex_destroy(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_destroy(status);
        }
    error_mutex_exec_context:
        if((status = pthread_mutex_destroy(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_destroy(status);
        }
    error:
        return -1;
}

int finish_global_variables(void) {
    int retval = 0, status;

    if((status = pthread_mutex_destroy(&MUTEX_EXEC_CONTEXT))) {
        log_error_pthread_mutex_destroy(status);
        retval = -1;
    }

    if((status = pthread_mutex_destroy(&MUTEX_EXECUTING))) {
        log_error_pthread_mutex_destroy(status);
        retval = -1;
    }

    if((status = pthread_mutex_destroy(&MUTEX_KERNEL_INTERRUPT))) {
        log_error_pthread_mutex_destroy(status);
        retval = -1;
    }

    return retval;
}

int read_module_config(t_config *MODULE_CONFIG) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "PUERTO_ESCUCHA_DISPATCH", "PUERTO_ESCUCHA_INTERRUPT", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

    CONNECTION_MEMORY = (t_Connection) {.client_type = CPU_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};

    SERVER_CPU_DISPATCH = (t_Server) {.server_type = CPU_DISPATCH_PORT_TYPE, .clients_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_DISPATCH")};
    CLIENT_KERNEL_CPU_DISPATCH = (t_Client) {.client_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .server = &SERVER_CPU_DISPATCH};

    SERVER_CPU_INTERRUPT = (t_Server) {.server_type = CPU_INTERRUPT_PORT_TYPE, .clients_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_INTERRUPT")};
    CLIENT_KERNEL_CPU_INTERRUPT = (t_Client) {.client_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .server = &SERVER_CPU_INTERRUPT};

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

        if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
            
            KERNEL_INTERRUPT = NONE_KERNEL_INTERRUPT;
        
        if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        payload_init(&SYSCALL_INSTRUCTION);

        if((status = pthread_mutex_lock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
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
            if(receive_exec_context(&EXEC_CONTEXT, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(EXIT_FAILURE);
            }
        if((status = pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        if((status = pthread_mutex_lock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }

            EXECUTING = 1;

        if((status = pthread_mutex_unlock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        log_trace(MODULE_LOGGER, "<%d:%d> Contexto de ejecucion recibido del proceso - Ciclo de instruccion ejecutando", PID, TID);

        while(1) {

            // Fetch
            log_info(MINIMAL_LOGGER, "## TID: %u - FETCH - Program Counter: %u", TID, EXEC_CONTEXT.cpu_registers.PC);
            cpu_fetch_next_instruction(&IR);
            if(IR == NULL) {
                log_error(MODULE_LOGGER, "Error al fetchear la instruccion");
                EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                break;
            }

            // Decode
            if((status = arguments_use(arguments, IR))) {
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

            if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
                log_error_pthread_mutex_lock(status);
                // TODO
            }
                if(KERNEL_INTERRUPT == CANCEL_KERNEL_INTERRUPT) {
                    if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                        log_error_pthread_mutex_unlock(status);
                        // TODO
                    }
                    EVICTION_REASON = CANCEL_EVICTION_REASON;
                    break;
                }
            if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }

            if(SYSCALL_CALLED) {
                EVICTION_REASON = SYSCALL_EVICTION_REASON;
                break;
            }

            if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
                log_error_pthread_mutex_lock(status);
                // TODO
            }
                if(KERNEL_INTERRUPT == QUANTUM_KERNEL_INTERRUPT) {
                    if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                        log_error_pthread_mutex_unlock(status);
                        // TODO
                    }
                    EVICTION_REASON = QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON;
                    break;
                }
            if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }
        }

        if((status = pthread_mutex_lock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
            EXECUTING = 0;
        if((status = pthread_mutex_unlock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        if((status = pthread_mutex_lock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
            log_info(MINIMAL_LOGGER, "## TID: %u - Actualizo Contexto Ejecución", TID);

            if(send_exec_context_update(PID, TID, EXEC_CONTEXT, CONNECTION_MEMORY.fd_connection)) {
                // TODO
                exit(EXIT_FAILURE);
            }

            if(send_thread_eviction(EVICTION_REASON, SYSCALL_INSTRUCTION, CLIENT_KERNEL_CPU_DISPATCH.fd_client)) {
                // TODO
                exit(EXIT_FAILURE);
            }
        if((status = pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            payload_destroy(&SYSCALL_INSTRUCTION);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }

    arguments_destroy(arguments);
}

void *kernel_cpu_interrupt_handler(void) {

    log_trace(MODULE_LOGGER, "Hilo de manejo de interrupciones de Kernel iniciado");

    e_Kernel_Interrupt kernel_interrupt;
    t_PID pid;
    t_TID tid;
    int status;

    while(1) {

        if(receive_kernel_interrupt(&kernel_interrupt, &pid, &tid, CLIENT_KERNEL_CPU_INTERRUPT.fd_client)) {
            // TODO
            exit(EXIT_FAILURE);
        }

        if((status = pthread_mutex_lock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
            if(!EXECUTING) {
                if((status = pthread_mutex_unlock(&MUTEX_EXECUTING))) {
                    log_error_pthread_mutex_unlock(status);
                    // TODO
                }
                continue;
            }
        if((status = pthread_mutex_unlock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        if((status = pthread_mutex_lock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
            if(pid != PID && tid != TID) {
                if((status = pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT))) {
                    log_error_pthread_mutex_unlock(status);
                    // TODO
                }
                continue;
            }
        if((status = pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        log_info(MINIMAL_LOGGER, "## Llega interrupción al puerto Interrupt");

        if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
            // Una forma de establecer prioridad entre interrupciones que se pisan, sólo va a quedar una
            if(KERNEL_INTERRUPT < kernel_interrupt)
                KERNEL_INTERRUPT = kernel_interrupt;
        if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

    }

    return NULL;
}

int cpu_fetch_next_instruction(char **line) {
    if(line == NULL) {
        errno = EINVAL;
        return -1;
    }

    if(send_instruction_request(PID, TID, EXEC_CONTEXT.cpu_registers.PC, CONNECTION_MEMORY.fd_connection)) {
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
        log_error(MODULE_LOGGER, "mmu: %s", strerror(EINVAL));
        errno = EINVAL;
        return -1;
    }

    // Verifico que no produzca overflow
    if(logical_address > (SIZE_MAX - EXEC_CONTEXT.base) || bytes > (SIZE_MAX - (EXEC_CONTEXT.base + logical_address))) {
        log_error(MODULE_LOGGER, "mmu: %s", strerror(ERANGE));
        errno = ERANGE;
        return -1;
    }

    // Calculo la dirección física
    size_t physical_address = EXEC_CONTEXT.base + logical_address;
    // La dirección lógica es el desplazamiento desde la base

    // Verifico que no haya segmentation fault
    if((physical_address + bytes) >= EXEC_CONTEXT.limit) {
        log_warning(MODULE_LOGGER, "mmu: %s", strerror(EFAULT));
        errno = EFAULT;
        return -1;
    }

    // Asigno la dirección física al puntero de destino
    *destination = physical_address;

    return 0;
}

int write_memory(size_t physical_address, void *source, size_t bytes) {
    if(source == NULL) {
        log_error(MODULE_LOGGER, "write_memory: %s", strerror(EINVAL));
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
        log_error(MODULE_LOGGER, "read_memory: %s", strerror(EINVAL));
        errno = EINVAL;
        return -1;
    }

    if(send_read_request(PID, TID, physical_address, bytes, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(EXIT_FAILURE);
    }

    void *buffer;
    size_t bufferSize;

    if(receive_data_with_expected_header(READ_REQUEST_HEADER, &buffer, &bufferSize, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(EXIT_FAILURE);
    }

    if(bufferSize != bytes) {
        log_error(MODULE_LOGGER, "read_memory: No coinciden los bytes leidos (%zd) con los que se esperaban leer (%zd)", bufferSize, bytes);
        errno = EIO;
        free(buffer);
        return -1;
    }

    memcpy(destination, buffer, bytes);
    free(buffer);

    return 0;
}