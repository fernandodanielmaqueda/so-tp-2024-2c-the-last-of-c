/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "cpu.h"

pthread_mutex_t MUTEX_KERNEL_INTERRUPT;
bool EXECUTING = 0;
t_PID PID;
t_TID TID;
e_Kernel_Interrupt KERNEL_INTERRUPT;
t_Exec_Context EXEC_CONTEXT;

e_Eviction_Reason EVICTION_REASON;

bool SYSCALL_CALLED;
t_Payload SYSCALL_INSTRUCTION;

int module(int argc, char *argv[])
{
    int status;

    MODULE_NAME = "cpu";
    MODULE_CONFIG_PATHNAME = "cpu.config";


	// Bloquea todas las señales para este y los hilos creados
	sigset_t set;
	if(sigfillset(&set)) {
		report_error_sigfillset();
		return EXIT_FAILURE;
	}
	if((status = pthread_sigmask(SIG_BLOCK, &set, NULL))) {
		report_error_pthread_sigmask(status);
		return EXIT_FAILURE;
	}


    pthread_t thread_main = pthread_self();


	// Crea hilo para manejar señales
	if((status = pthread_create(&THREAD_SIGNAL_MANAGER, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main))) {
		report_error_pthread_create(status);
		return EXIT_FAILURE;
	}
	pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &THREAD_SIGNAL_MANAGER);


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
	if((status = pthread_mutex_init(&MUTEX_LOGGERS, NULL))) {
		report_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_LOGGERS);

	if(logger_init(&MODULE_LOGGER, MODULE_LOGGER_INIT_ENABLED, MODULE_LOGGER_PATHNAME, MODULE_LOGGER_NAME, MODULE_LOGGER_INIT_ACTIVE_CONSOLE, MODULE_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &MODULE_LOGGER);

	if(logger_init(&MINIMAL_LOGGER, MINIMAL_LOGGER_INIT_ENABLED, MINIMAL_LOGGER_PATHNAME, MINIMAL_LOGGER_NAME, MINIMAL_LOGGER_ACTIVE_CONSOLE, MINIMAL_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &MINIMAL_LOGGER);

	if(logger_init(&SOCKET_LOGGER, SOCKET_LOGGER_INIT_ENABLED, SOCKET_LOGGER_PATHNAME, SOCKET_LOGGER_NAME, SOCKET_LOGGER_INIT_ACTIVE_CONSOLE, SOCKET_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &SOCKET_LOGGER);

	if(logger_init(&SERIALIZE_LOGGER, SERIALIZE_LOGGER_INIT_ENABLED, SERIALIZE_LOGGER_PATHNAME, SERIALIZE_LOGGER_NAME, SERIALIZE_LOGGER_INIT_ACTIVE_CONSOLE, SERIALIZE_LOGGER_INIT_LOG_LEVEL)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) logger_destroy, (void *) &SERIALIZE_LOGGER);


    // MUTEX_KERNEL_INTERRUPT
    if((status = pthread_mutex_init(&MUTEX_KERNEL_INTERRUPT, NULL))) {
        report_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_KERNEL_INTERRUPT);


	// Sockets
	pthread_cleanup_push((void (*)(void *)) finish_sockets, NULL);
	initialize_sockets();

    if((status = pthread_create(&(CLIENT_KERNEL_CPU_INTERRUPT.socket_client.bool_thread.thread), NULL, (void *(*)(void *)) kernel_cpu_interrupt_handler, NULL))) {
        report_error_pthread_create(status);
         exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &(CLIENT_KERNEL_CPU_INTERRUPT.socket_client.bool_thread.thread));


    log_debug_r(&MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);


    instruction_cycle();


	// Cleanup
	pthread_cleanup_pop(1); // CLIENT_KERNEL_CPU_INTERRUPT
	pthread_cleanup_pop(1); // Sockets
	pthread_cleanup_pop(1); // MUTEX_KERNEL_INTERRUPT
	pthread_cleanup_pop(1); // SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // SOCKET_LOGGER
	pthread_cleanup_pop(1); // MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MODULE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_LOGGERS
	pthread_cleanup_pop(1); // MODULE_CONFIG
	pthread_cleanup_pop(1); // THREAD_SIGNAL_MANAGER

    return EXIT_SUCCESS;
}

int read_module_config(t_config *MODULE_CONFIG) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "PUERTO_ESCUCHA_DISPATCH", "PUERTO_ESCUCHA_INTERRUPT", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

    CONNECTION_MEMORY = (t_Connection) {.socket_connection.fd = -1, .socket_connection.bool_thread.running = false, .client_type = CPU_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};

    SERVER_CPU_DISPATCH = (t_Server) {.socket_listen.fd = -1, .socket_listen.bool_thread.running = false, .server_type = CPU_DISPATCH_PORT_TYPE, .clients_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_DISPATCH")};
    CLIENT_KERNEL_CPU_DISPATCH = (t_Client) {.socket_client.fd = -1, .socket_client.bool_thread.running = false, .client_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .server = &SERVER_CPU_DISPATCH};

    SERVER_CPU_INTERRUPT = (t_Server) {.socket_listen.fd = -1, .socket_listen.bool_thread.running = false, .server_type = CPU_INTERRUPT_PORT_TYPE, .clients_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_INTERRUPT")};
    CLIENT_KERNEL_CPU_INTERRUPT = (t_Client) {.socket_client.fd = -1, .socket_client.bool_thread.running = false, .client_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .server = &SERVER_CPU_INTERRUPT};

    LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));

    return 0;
}

void instruction_cycle(void)
{
    int result;

    char *ir = NULL;
    e_CPU_OpCode cpu_opcode;
    e_Kernel_Interrupt kernel_interrupt;
    int status;

    t_Arguments *arguments = arguments_create(MAX_CPU_INSTRUCTION_ARGUMENTS);
    if(arguments == NULL) {
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) arguments_destroy, arguments);

    while(1) {

        payload_init(&SYSCALL_INSTRUCTION);
        pthread_cleanup_push((void (*)(void *)) payload_destroy, &SYSCALL_INSTRUCTION);

            if(receive_pid_and_tid_with_expected_header(THREAD_DISPATCH_HEADER, &PID, &TID, CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd)) {
                log_error_r(&MODULE_LOGGER, "[%d] Error al recibir dispatch de hilo de [Cliente] %s", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type]);
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER, "[%d] Se recibe dispatch de hilo de [Cliente] %s [PID: %u - TID: %u]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID);

            if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
                report_error_pthread_mutex_lock(status);
                exit_sigint();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_KERNEL_INTERRUPT);
                EXECUTING = true;
                KERNEL_INTERRUPT = NONE_KERNEL_INTERRUPT;
            pthread_cleanup_pop(0);
            if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                report_error_pthread_mutex_unlock(status);
                exit_sigint();
            }

            if(send_header(THREAD_DISPATCH_HEADER, CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd)) {
                log_error_r(&MODULE_LOGGER, "[%d] Error al enviar confirmación de dispatch de hilo a [Cliente] %s [PID: %u - TID: %u]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID);
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER, "[%d] Se envía confirmación de dispatch de hilo a [Cliente] %s [PID: %u - TID: %u]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID);

            log_info_r(&MINIMAL_LOGGER, "## TID: %u - Solicito Contexto Ejecución", TID);

            // Esto funciona como solicitud a memoria para que me mande el contexto de ejecución
            if(send_pid_and_tid_with_header(EXEC_CONTEXT_REQUEST_HEADER, PID, TID, CONNECTION_MEMORY.socket_connection.fd)) {
                log_error_r(&MODULE_LOGGER, "[%d] Error al enviar solicitud de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER, "[%d] Se envía solicitud de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);

            if(receive_result_with_expected_header(EXEC_CONTEXT_REQUEST_HEADER, &result, CONNECTION_MEMORY.socket_connection.fd)) {
                log_error_r(&MODULE_LOGGER, "[%d] Error al recibir resultado de solicitud de contexto de ejecución de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER, "[%d] Se recibe resultado de solicitud de contexto de ejecución de [Servidor] %s [PID: %u - TID: %u - Resultado: %d]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, result);

            if(result) {
                goto cleanup_syscall_instruction;
            }

            // Recibo la respuesta de memoria con el contexto de ejecución
            if(receive_exec_context(&EXEC_CONTEXT, CONNECTION_MEMORY.socket_connection.fd)) {
                log_error_r(&MODULE_LOGGER, "[%d] Error al recibir contexto de ejecución de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER,
            "[%d] Se recibe contexto de ejecución de [Servidor] %s [PID: %u - TID: %u"
            " - PC: %u"
            " - AX: %u"
            " - BX: %u"
            " - CX: %u"
            " - DX: %u"
            " - EX: %u"
            " - FX: %u"
            " - GX: %u"
            " - HX: %u"
            " - base: %u"
            " - limit: %u]"
            , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID
            , EXEC_CONTEXT.cpu_registers.PC
            , EXEC_CONTEXT.cpu_registers.AX
            , EXEC_CONTEXT.cpu_registers.BX
            , EXEC_CONTEXT.cpu_registers.CX
            , EXEC_CONTEXT.cpu_registers.DX
            , EXEC_CONTEXT.cpu_registers.EX
            , EXEC_CONTEXT.cpu_registers.FX
            , EXEC_CONTEXT.cpu_registers.GX
            , EXEC_CONTEXT.cpu_registers.HX
            , EXEC_CONTEXT.base
            , EXEC_CONTEXT.limit
            );

            bool evict = false;
            while(!evict) {

                // Fetch
                log_info_r(&MINIMAL_LOGGER, "## TID: %u - FETCH - Program Counter: %u", TID, EXEC_CONTEXT.cpu_registers.PC);
                cpu_fetch_next_instruction(&ir);
                if(ir == NULL) {
                    log_warning_r(&MODULE_LOGGER, "Error en el fetch de la instrucción [PID: %u - TID: %u - PC: %u]", PID, TID, EXEC_CONTEXT.cpu_registers.PC);
                    evict = true;
                    EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                    goto cleanup_ir;
                }
                pthread_cleanup_push((void (*)(void *)) free, ir);

                // Decode
                if((status = arguments_use(arguments, ir))) {
                    switch(errno) {
                        case E2BIG:
                            log_warning_r(&MODULE_LOGGER, "%s: Demasiados argumentos en la instruccion", ir);
                            evict = true;
                            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                            goto cleanup_ir;
                        default:
                            log_error_r(&MODULE_LOGGER, "arguments_use: %s", strerror(errno));
                            exit_sigint();
                    }
                }

                pthread_cleanup_push((void (*)(void *)) arguments_remove, arguments);

                if(decode_instruction(arguments->argv[0], &cpu_opcode)) {
                    log_warning_r(&MODULE_LOGGER, "%s: Error en el decode de la instrucción [PID: %u - TID: %u - PC: %u]", arguments->argv[0], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
                    evict = true;
                    EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                    goto cleanup_arguments_remove;
                }

                // Execute
                if((status = CPU_OPERATIONS[cpu_opcode].function(arguments->argc, arguments->argv))) {
                    log_warning_r(&MODULE_LOGGER, "%s: Error en la ejecución de la instrucción [PID: %u - TID: %u - PC: %u]", arguments->argv[0], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
                    evict = true;
                    // EVICTION_REASON ya debe ser asignado por la instrucción cuando falla
                    goto cleanup_arguments_remove;
                }

                cleanup_arguments_remove:
                pthread_cleanup_pop(1); // arguments_remove
                cleanup_ir:
                pthread_cleanup_pop(1); // ir

                if(evict) {
                    goto eviction;
                }

                // Check Interrupts

                if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
                    report_error_pthread_mutex_lock(status);
                    exit_sigint();
                }
                pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_KERNEL_INTERRUPT);
                    kernel_interrupt = KERNEL_INTERRUPT;
                    if((kernel_interrupt > NONE_KERNEL_INTERRUPT) || SYSCALL_CALLED) {
                        EXECUTING = false;
                    }
                pthread_cleanup_pop(0); // MUTEX_KERNEL_INTERRUPT
                if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                    report_error_pthread_mutex_unlock(status);
                    exit_sigint();
                }

                // Prioritary syscalls
                switch(cpu_opcode) {

                    case PROCESS_EXIT_CPU_OPCODE:
                    case THREAD_EXIT_CPU_OPCODE:
                        evict = true;
                        EVICTION_REASON = EXIT_EVICTION_REASON;
                        goto eviction;

                    default:
                        break;

                }

                if(kernel_interrupt == KILL_KERNEL_INTERRUPT) {
                    evict = true;
                    EVICTION_REASON = KILL_EVICTION_REASON;
                    goto eviction;
                }

                if(SYSCALL_CALLED) {
                    evict = true;
                    EVICTION_REASON = SYSCALL_EVICTION_REASON;
                    goto eviction;
                }

                if(kernel_interrupt == QUANTUM_KERNEL_INTERRUPT) {
                    evict = true;
                    EVICTION_REASON = QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON;
                    goto eviction;
                }
            }

            eviction:

            log_info_r(&MINIMAL_LOGGER, "## TID: %u - Actualizo Contexto Ejecución", TID);

            if(send_exec_context_update(PID, TID, EXEC_CONTEXT, CONNECTION_MEMORY.socket_connection.fd)) {
                log_error_r(&MODULE_LOGGER,
                    "[%d] Error al enviar actualización de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u"
                    " - PC: %u"
                    " - AX: %u"
                    " - BX: %u"
                    " - CX: %u"
                    " - DX: %u"
                    " - EX: %u"
                    " - FX: %u"
                    " - GX: %u"
                    " - HX: %u"
                    " - base: %u"
                    " - limit: %u]"
                    , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID
                    , EXEC_CONTEXT.cpu_registers.PC
                    , EXEC_CONTEXT.cpu_registers.AX
                    , EXEC_CONTEXT.cpu_registers.BX
                    , EXEC_CONTEXT.cpu_registers.CX
                    , EXEC_CONTEXT.cpu_registers.DX
                    , EXEC_CONTEXT.cpu_registers.EX
                    , EXEC_CONTEXT.cpu_registers.FX
                    , EXEC_CONTEXT.cpu_registers.GX
                    , EXEC_CONTEXT.cpu_registers.HX
                    , EXEC_CONTEXT.base
                    , EXEC_CONTEXT.limit
                );
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER,
                "[%d] Se envía actualización de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u"
                " - PC: %u"
                " - AX: %u"
                " - BX: %u"
                " - CX: %u"
                " - DX: %u"
                " - EX: %u"
                " - FX: %u"
                " - GX: %u"
                " - HX: %u"
                " - base: %u"
                " - limit: %u]"
                , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID
                , EXEC_CONTEXT.cpu_registers.PC
                , EXEC_CONTEXT.cpu_registers.AX
                , EXEC_CONTEXT.cpu_registers.BX
                , EXEC_CONTEXT.cpu_registers.CX
                , EXEC_CONTEXT.cpu_registers.DX
                , EXEC_CONTEXT.cpu_registers.EX
                , EXEC_CONTEXT.cpu_registers.FX
                , EXEC_CONTEXT.cpu_registers.GX
                , EXEC_CONTEXT.cpu_registers.HX
                , EXEC_CONTEXT.base
                , EXEC_CONTEXT.limit
            );

            int result;
            if(receive_result_with_expected_header(EXEC_CONTEXT_UPDATE_HEADER, &result, CONNECTION_MEMORY.socket_connection.fd)) {
                log_error_r(&MODULE_LOGGER, "[%d] Error al recibir resultado de actualización de contexto de ejecución de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER, "[%d] Se recibe resultado de actualización de contexto de ejecución de [Servidor] %s [PID: %u - TID: %u - Resultado: %d]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, result);

            if(send_thread_eviction(EVICTION_REASON, SYSCALL_INSTRUCTION, CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd)) {
                log_error_r(&MODULE_LOGGER, "[%d] Error al enviar desalojo de hilo a [Cliente] %s [PID: %u - TID: %u - Motivo: :%s]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID, EVICTION_REASON_NAMES[EVICTION_REASON]);
                exit_sigint();
            }
            log_trace_r(&MODULE_LOGGER, "[%d] Se envía desalojo de hilo a [Cliente] %s [PID: %u - TID: %u - Motivo: %s]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID, EVICTION_REASON_NAMES[EVICTION_REASON]);

        cleanup_syscall_instruction:
        pthread_cleanup_pop(1); // SYSCALL_INSTRUCTION
    }

    pthread_cleanup_pop(1); // arguments_destroy
}

void *kernel_cpu_interrupt_handler(void) {

    log_trace_r(&MODULE_LOGGER, "Hilo de manejo de interrupciones de Kernel iniciado");

    e_Kernel_Interrupt kernel_interrupt;
    t_PID pid;
    t_TID tid;
    int status;

    while(1) {

        if(receive_kernel_interrupt(&kernel_interrupt, &pid, &tid, CLIENT_KERNEL_CPU_INTERRUPT.socket_client.fd)) {
            log_error_r(&MODULE_LOGGER, "[%d]: Error al recibir interrupción de [Cliente] %s", CLIENT_KERNEL_CPU_INTERRUPT.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_INTERRUPT.client_type]);
            exit_sigint();
        }
        log_trace_r(&MODULE_LOGGER, "[%d]: Se recibe interrupción de [Cliente] %s [PID: %u - TID: %u - Interrupción: %s]", CLIENT_KERNEL_CPU_INTERRUPT.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_INTERRUPT.client_type], pid, tid, KERNEL_INTERRUPT_NAMES[kernel_interrupt]);

        if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
            report_error_pthread_mutex_lock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_KERNEL_INTERRUPT);
            if((!EXECUTING) || (pid != PID) || (tid != TID)) {
                log_trace_r(&MODULE_LOGGER, "Interrupción recibida pero el hilo no se encuentra en ejecución [PID: %u - TID: %u - Interrupción: %s]", pid, tid, KERNEL_INTERRUPT_NAMES[kernel_interrupt]);
            }
            else {
                // Una forma de establecer prioridad entre interrupciones que se pisan, sólo va a quedar una
                if(KERNEL_INTERRUPT < kernel_interrupt) {
                    KERNEL_INTERRUPT = kernel_interrupt;
                }
                log_info_r(&MINIMAL_LOGGER, "## Llega interrupción al puerto Interrupt");
            }
        pthread_cleanup_pop(0); // MUTEX_KERNEL_INTERRUPT
        if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
            report_error_pthread_mutex_unlock(status);
            exit_sigint();
        }
    }

    return NULL;
}

void cpu_fetch_next_instruction(char **line) {

    if(send_instruction_request(PID, TID, EXEC_CONTEXT.cpu_registers.PC, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d]: Error al enviar solicitud de instrucción a [Servidor] %s [PID: %u - TID: %u - PC: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d]: Se envía solicitud de instrucción a [Servidor] %s [PID: %u - TID: %u - PC: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC);

    if(receive_text_with_expected_header(INSTRUCTION_REQUEST_HEADER, line, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d]: Error al recibir instrucción de [Servidor] %s [PID: %u - TID: %u - PC: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d]: Se recibe instrucción de [Servidor] %s [PID: %u - TID: %u - PC: %u - Instrucción: %s]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC, *line);
}

int mmu(size_t logical_address, size_t bytes, size_t *destination) {
    // Verifico que el puntero de destino no sea nulo
    if(destination == NULL) {
        log_warning_r(&MODULE_LOGGER, "mmu: %s", strerror(EINVAL));
        errno = EINVAL;
        return -1;
    }

    // Verifico que no produzca overflow
    if(logical_address > (SIZE_MAX - EXEC_CONTEXT.base) || bytes > (SIZE_MAX - (EXEC_CONTEXT.base + logical_address))) {
        log_warning_r(&MODULE_LOGGER, "mmu: %s", strerror(ERANGE));
        errno = ERANGE;
        return -1;
    }

    // Calculo la dirección física
    size_t physical_address = EXEC_CONTEXT.base + logical_address;
    // La dirección lógica es el desplazamiento desde la base

    // Verifico que no haya segmentation fault
    if((physical_address + bytes) >= (EXEC_CONTEXT.base + EXEC_CONTEXT.limit)) {
        log_warning_r(&MODULE_LOGGER, "mmu: %s", strerror(EFAULT));
        errno = EFAULT;
        return -1;
    }

    // Asigno la dirección física al puntero de destino
    *destination = physical_address;

    return 0;
}

void request_memory_write(size_t physical_address, void *source, size_t bytes, int *result) {
    if(source == NULL) {
        log_error_r(&MODULE_LOGGER, "request_memory_write: %s", strerror(EINVAL));
        exit_sigint();
    }

    char *data_string = mem_hexstring(source, bytes);
    pthread_cleanup_push((void (*)(void *)) free, data_string);

        if(send_write_request(PID, TID, physical_address, source, bytes, CONNECTION_MEMORY.socket_connection.fd)) {
            log_error_r(&MODULE_LOGGER, 
            "[%d] Error al enviar solicitud de escritura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]"
            "%s"
            , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes
            , data_string
            );
            exit_sigint();
        }
        log_trace_r(&MODULE_LOGGER,
          "[%d] Se envía solicitud de escritura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]"
          "%s"
          , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes
          , data_string
        );

    pthread_cleanup_pop(1); // data_string

    if(receive_result_with_expected_header(WRITE_REQUEST_HEADER, result, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d] Error al recibir resultado de escritura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d] Se recibe resultado de escritura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu - Resultado: %d]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes, *result);
}

void request_memory_read(size_t physical_address, void *destination, size_t bytes, int *result) {
    if(destination == NULL) {
        log_error_r(&MODULE_LOGGER, "request_memory_read: %s", strerror(EINVAL));
        exit_sigint();
    }

    if(send_read_request(PID, TID, physical_address, bytes, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d] Error al enviar solicitud de lectura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d] Se envía solicitud de lectura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);

    void *buffer = NULL;
    size_t bufferSize = 0;

    if(receive_data_with_expected_header(READ_REQUEST_HEADER, &buffer, &bufferSize, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d] Error al recibir resultado de lectura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) free, buffer);

        char *data_string = mem_hexstring(buffer, bufferSize);
        pthread_cleanup_push((void (*)(void *)) free, data_string);
            log_trace_r(&MODULE_LOGGER,
            "[%d] Se recibe resultado de lectura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]"
            "%s"
            , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes
            , data_string
            );
        pthread_cleanup_pop(1); // data_string

        if(bufferSize != bytes) {
            log_warning_r(&MODULE_LOGGER, "No coinciden los bytes leidos (%zu) con los que se esperaban leer (%zu) en espacio de usuario", bufferSize, bytes);
            *result = -1;
            goto cleanup_buffer;
        }

        memcpy(destination, buffer, bytes);

        *result = 0;
    
    cleanup_buffer:
    pthread_cleanup_pop(1); // buffer
}