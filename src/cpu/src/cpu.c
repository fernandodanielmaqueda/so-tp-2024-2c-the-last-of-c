/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "cpu.h"

t_PID PID;
t_TID TID;
t_Exec_Context EXEC_CONTEXT;
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
    int status;

    MODULE_NAME = "cpu";
    MODULE_LOG_PATHNAME = "cpu.log";
    MODULE_CONFIG_PATHNAME = "cpu.config";


	// Bloquea todas las señales para este y los hilos creados
	sigset_t set;
	if(sigfillset(&set)) {
		log_error_sigfillset();
		return EXIT_FAILURE;
	}
	if((status = pthread_sigmask(SIG_BLOCK, &set, NULL))) {
		log_error_pthread_sigmask(status);
		return EXIT_FAILURE;
	}


    pthread_t thread_main = pthread_self();


	// Crea hilo para manejar señales
	if((status = pthread_create(&THREAD_SIGNAL_MANAGER, NULL, (void *(*)(void *)) signal_manager, (void *) &thread_main))) {
		log_error_pthread_create(status);
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
	if((status = pthread_mutex_init(&MUTEX_MINIMAL_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_MINIMAL_LOGGER);
	if(initialize_logger(&MINIMAL_LOGGER, MINIMAL_LOG_PATHNAME, "Minimal")) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MINIMAL_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_MODULE_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_MODULE_LOGGER);
	if(initialize_logger(&MODULE_LOGGER, MODULE_LOG_PATHNAME, MODULE_NAME)) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &MODULE_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_SOCKET_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_SOCKET_LOGGER);
	if(initialize_logger(&SOCKET_LOGGER, SOCKET_LOG_PATHNAME, "Socket")) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SOCKET_LOGGER);

	if((status = pthread_mutex_init(&MUTEX_SERIALIZE_LOGGER, NULL))) {
		log_error_pthread_mutex_init(status);
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_SERIALIZE_LOGGER);
	if(initialize_logger(&SERIALIZE_LOGGER, SERIALIZE_LOG_PATHNAME, "Serialize")) {
		exit_sigint();
	}
	pthread_cleanup_push((void (*)(void *)) finish_logger, (void *) &SERIALIZE_LOGGER);


    // MUTEX_EXEC_CONTEXT
    if((status = pthread_mutex_init(&MUTEX_EXEC_CONTEXT, NULL))) {
        log_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_EXEC_CONTEXT);


    // MUTEX_EXECUTING
    if((status = pthread_mutex_init(&MUTEX_EXECUTING, NULL))) {
        log_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_EXECUTING);


    // MUTEX_KERNEL_INTERRUPT
    if((status = pthread_mutex_init(&MUTEX_KERNEL_INTERRUPT, NULL))) {
        log_error_pthread_mutex_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_mutex_destroy, (void *) &MUTEX_KERNEL_INTERRUPT);


	// Sockets
	pthread_cleanup_push((void (*)(void *)) finish_sockets, NULL);
	initialize_sockets();

    if((status = pthread_create(&(CLIENT_KERNEL_CPU_INTERRUPT.socket_client.bool_thread.thread), NULL, (void *(*)(void *)) kernel_cpu_interrupt_handler, NULL))) {
        log_error_pthread_create(status);
         exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) cancel_and_join_pthread, (void *) &(CLIENT_KERNEL_CPU_INTERRUPT.socket_client.bool_thread.thread));


    log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);


    instruction_cycle();


	// Cleanup
	pthread_cleanup_pop(1); // CLIENT_KERNEL_CPU_INTERRUPT
	pthread_cleanup_pop(1); // Sockets
	pthread_cleanup_pop(1); // MUTEX_KERNEL_INTERRUPT
	pthread_cleanup_pop(1); // MUTEX_EXECUTING
	pthread_cleanup_pop(1); // MUTEX_EXEC_CONTEXT
	pthread_cleanup_pop(1); // SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_SERIALIZE_LOGGER
	pthread_cleanup_pop(1); // SOCKET_LOGGER
	pthread_cleanup_pop(1); // MUTEX_SOCKET_LOGGER
	pthread_cleanup_pop(1); // MODULE_LOGGER
	pthread_cleanup_pop(1); // MUTEX_MODULE_LOGGER
	pthread_cleanup_pop(1); // MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MUTEX_MINIMAL_LOGGER
	pthread_cleanup_pop(1); // MODULE_CONFIG
	pthread_cleanup_pop(1); // THREAD_SIGNAL_MANAGER

    return EXIT_SUCCESS;
}

int read_module_config(t_config *MODULE_CONFIG) {

    if(!config_has_properties(MODULE_CONFIG, "IP_MEMORIA", "PUERTO_MEMORIA", "PUERTO_ESCUCHA_DISPATCH", "PUERTO_ESCUCHA_INTERRUPT", "LOG_LEVEL", NULL)) {
        fprintf(stderr, "%s: El archivo de configuración no contiene todas las claves necesarias\n", MODULE_CONFIG_PATHNAME);
        return -1;
    }

    CONNECTION_MEMORY = (t_Connection) {.client_type = CPU_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};

    SERVER_CPU_DISPATCH = (t_Server) {.server_type = CPU_DISPATCH_PORT_TYPE, .clients_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_DISPATCH")};
    CLIENT_KERNEL_CPU_DISPATCH = (t_Client) {.socket_client.bool_thread.running = false, .client_type = KERNEL_CPU_DISPATCH_PORT_TYPE, .server = &SERVER_CPU_DISPATCH};

    SERVER_CPU_INTERRUPT = (t_Server) {.server_type = CPU_INTERRUPT_PORT_TYPE, .clients_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .port = config_get_string_value(MODULE_CONFIG, "PUERTO_ESCUCHA_INTERRUPT")};
    CLIENT_KERNEL_CPU_INTERRUPT = (t_Client) {.socket_client.bool_thread.running = false, .client_type = KERNEL_CPU_INTERRUPT_PORT_TYPE, .server = &SERVER_CPU_INTERRUPT};

    LOG_LEVEL = log_level_from_string(config_get_string_value(MODULE_CONFIG, "LOG_LEVEL"));

    return 0;
}

void instruction_cycle(void)
{

    char *ir = NULL;
    e_CPU_OpCode cpu_opcode;
    int status;

    t_Arguments *arguments = arguments_create(MAX_CPU_INSTRUCTION_ARGUMENTS);
    if(arguments == NULL) {
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) arguments_destroy, arguments);

    while(1) {

        if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_lock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_KERNEL_INTERRUPT);
            KERNEL_INTERRUPT = NONE_KERNEL_INTERRUPT;
        pthread_cleanup_pop(0);
        if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_unlock(status);
            exit_sigint();
        }

        payload_init(&SYSCALL_INSTRUCTION);
        pthread_cleanup_push((void (*)(void *)) payload_destroy, &SYSCALL_INSTRUCTION);

            if((status = pthread_mutex_lock(&MUTEX_EXEC_CONTEXT))) {
                log_error_pthread_mutex_lock(status);
                exit_sigint();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_EXEC_CONTEXT);

                if(receive_pid_and_tid_with_expected_header(THREAD_DISPATCH_HEADER, &PID, &TID, CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd)) {
                    log_error(MODULE_LOGGER, "[%d] Error al recibir dispatch de hilo de [Cliente] %s", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type]);
                    exit_sigint();
                }
                log_trace(MODULE_LOGGER, "[%d] Se recibe dispatch de hilo de [Cliente] %s [PID: %u - TID: %u]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID);

                log_info(MINIMAL_LOGGER, "## TID: %u - Solicito Contexto Ejecución", TID);

                // Esto funciona como solicitud a memoria para que me mande el contexto de ejecución
                if(send_pid_and_tid_with_header(EXEC_CONTEXT_REQUEST_HEADER, PID, TID, CONNECTION_MEMORY.socket_connection.fd)) {
                    log_error(MODULE_LOGGER, "[%d] Error al enviar solicitud de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);
                    exit_sigint();
                }
                log_trace(MODULE_LOGGER, "[%d] Se envía solicitud de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);

                // Recibo la respuesta de memoria con el contexto de ejecución
                if(receive_exec_context(&EXEC_CONTEXT, CONNECTION_MEMORY.socket_connection.fd)) {
                    log_error(MODULE_LOGGER, "[%d] Error al recibir contexto de ejecución de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);
                    exit_sigint();
                }
                log_trace(MODULE_LOGGER,
                "[%d] Se recibe contexto de ejecución de [Servidor] %s [PID: %u - TID: %u]\n"
                "* PC: %u\n"
                "* AX: %u\n"
                "* BX: %u\n"
                "* CX: %u\n"
                "* DX: %u\n"
                "* EX: %u\n"
                "* FX: %u\n"
                "* GX: %u\n"
                "* HX: %u\n"
                "* base: %u\n"
                "* limit: %u"
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

            pthread_cleanup_pop(0); // MUTEX_EXEC_CONTEXT
            if((status = pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT))) {
                log_error_pthread_mutex_unlock(status);
                exit_sigint();
            }

            if((status = pthread_mutex_lock(&MUTEX_EXECUTING))) {
                log_error_pthread_mutex_lock(status);
                exit_sigint();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_EXECUTING);
                EXECUTING = 1;
            pthread_cleanup_pop(0); // MUTEX_EXECUTING
            if((status = pthread_mutex_unlock(&MUTEX_EXECUTING))) {
                log_error_pthread_mutex_unlock(status);
                exit_sigint();
            }

            bool evict = false;
            while(!evict) {

                // Fetch
                log_info(MINIMAL_LOGGER, "## TID: %u - FETCH - Program Counter: %u", TID, EXEC_CONTEXT.cpu_registers.PC);
                cpu_fetch_next_instruction(&ir);
                if(ir == NULL) {
                    log_warning(MODULE_LOGGER, "Error en el fetch de la instrucción [PID: %u - TID: %u - PC: %u]", PID, TID, EXEC_CONTEXT.cpu_registers.PC);
                    evict = true;
                    EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                    goto cleanup_ir;
                }
                pthread_cleanup_push((void (*)(void *)) free, ir);

                // Decode
                if((status = arguments_use(arguments, ir))) {
                    switch(errno) {
                        case E2BIG:
                            log_error(MODULE_LOGGER, "%s: Demasiados argumentos en la instruccion", ir);
                            evict = true;
                            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                            goto cleanup_ir;
                        default:
                            log_error(MODULE_LOGGER, "arguments_use: %s", strerror(errno));
                            exit_sigint();
                    }
                }

                pthread_cleanup_push((void (*)(void *)) arguments_remove, arguments);

                if(decode_instruction(arguments->argv[0], &cpu_opcode)) {
                    log_warning(MODULE_LOGGER, "%s: Error en el decode de la instrucción [PID: %u - TID: %u - PC: %u]", arguments->argv[0], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
                    evict = true;
                    EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                    goto cleanup_arguments_remove;
                }

                // Execute
                if((status = CPU_OPERATIONS[cpu_opcode].function(arguments->argc, arguments->argv))) {
                    log_warning(MODULE_LOGGER, "%s: Error en la ejecución de la instrucción [PID: %u - TID: %u - PC: %u]", arguments->argv[0], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
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
                switch(cpu_opcode) {

                    case PROCESS_EXIT_CPU_OPCODE:
                    case THREAD_EXIT_CPU_OPCODE:
                        EVICTION_REASON = EXIT_EVICTION_REASON;
                        goto eviction;

                    default:
                        break;

                }

                if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
                    log_error_pthread_mutex_lock(status);
                    exit_sigint();
                }
                pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_KERNEL_INTERRUPT);
                    if(KERNEL_INTERRUPT == KILL_KERNEL_INTERRUPT) {
                        evict = true;
                        EVICTION_REASON = KILL_EVICTION_REASON;
                    }
                pthread_cleanup_pop(0); // MUTEX_KERNEL_INTERRUPT
                if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                    log_error_pthread_mutex_unlock(status);
                    exit_sigint();
                }

                if(evict) {
                    goto eviction;
                }

                if(SYSCALL_CALLED) {
                    evict = true;
                    EVICTION_REASON = SYSCALL_EVICTION_REASON;
                    goto eviction;
                }

                if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
                    log_error_pthread_mutex_lock(status);
                    exit_sigint();
                }
                pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_KERNEL_INTERRUPT);
                    if(KERNEL_INTERRUPT == QUANTUM_KERNEL_INTERRUPT) {
                        evict = true;
                        EVICTION_REASON = QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON;
                        goto eviction;
                    }
                pthread_cleanup_pop(0); // MUTEX_KERNEL_INTERRUPT
                if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
                    log_error_pthread_mutex_unlock(status);
                    exit_sigint();
                }

                if(evict) {
                    goto eviction;
                }
            }

            eviction:

            if((status = pthread_mutex_lock(&MUTEX_EXECUTING))) {
                log_error_pthread_mutex_lock(status);
                exit_sigint();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_EXECUTING);
                EXECUTING = 0;
            pthread_cleanup_pop(0); // MUTEX_EXECUTING
            if((status = pthread_mutex_unlock(&MUTEX_EXECUTING))) {
                log_error_pthread_mutex_unlock(status);
                exit_sigint();
            }

            if((status = pthread_mutex_lock(&MUTEX_EXEC_CONTEXT))) {
                log_error_pthread_mutex_lock(status);
                exit_sigint();
            }
            pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_EXEC_CONTEXT);

                log_info(MINIMAL_LOGGER, "## TID: %u - Actualizo Contexto Ejecución", TID);

                if(send_exec_context_update(PID, TID, EXEC_CONTEXT, CONNECTION_MEMORY.socket_connection.fd)) {
                    log_error(MODULE_LOGGER,
                      "[%d] Error al enviar actualización de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u]\n"
                      "* PC: %u\n"
                      "* AX: %u\n"
                      "* BX: %u\n"
                      "* CX: %u\n"
                      "* DX: %u\n"
                      "* EX: %u\n"
                      "* FX: %u\n"
                      "* GX: %u\n"
                      "* HX: %u\n"
                      "* base: %u\n"
                      "* limit: %u"
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
                log_trace(MODULE_LOGGER,
                  "[%d] Se envía actualización de contexto de ejecución a [Servidor] %s [PID: %u - TID: %u]\n"
                  "* PC: %u\n"
                  "* AX: %u\n"
                  "* BX: %u\n"
                  "* CX: %u\n"
                  "* DX: %u\n"
                  "* EX: %u\n"
                  "* FX: %u\n"
                  "* GX: %u\n"
                  "* HX: %u\n"
                  "* base: %u\n"
                  "* limit: %u"
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

                if(receive_expected_header(EXEC_CONTEXT_UPDATE_HEADER, CONNECTION_MEMORY.socket_connection.fd)) {
                    log_error(MODULE_LOGGER, "[%d] Error al recibir confirmación de actualización de contexto de ejecución de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);
                    exit_sigint();
                }
                log_trace(MODULE_LOGGER, "[%d] Se recibe confirmación de actualización de contexto de ejecución de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID);

                if(send_thread_eviction(EVICTION_REASON, SYSCALL_INSTRUCTION, CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd)) {
                    log_error(MODULE_LOGGER, "[%d] Error al enviar desalojo de hilo a [Cliente] %s [PID: %u - TID: %u - Motivo: :%s]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID, EVICTION_REASON_NAMES[EVICTION_REASON]);
                    exit_sigint();
                }
                log_trace(MODULE_LOGGER, "[%d] Se envía desalojo de hilo a [Cliente] %s [PID: %u - TID: %u - Motivo: %s]", CLIENT_KERNEL_CPU_DISPATCH.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_DISPATCH.client_type], PID, TID, EVICTION_REASON_NAMES[EVICTION_REASON]);

            pthread_cleanup_pop(0); // MUTEX_EXEC_CONTEXT
            if((status = pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT))) {
                log_error_pthread_mutex_unlock(status);
                exit_sigint();
            }

        pthread_cleanup_pop(1); // SYSCALL_INSTRUCTION
    }

    pthread_cleanup_pop(1); // arguments_destroy
}

void *kernel_cpu_interrupt_handler(void) {

    log_trace(MODULE_LOGGER, "Hilo de manejo de interrupciones de Kernel iniciado");

    e_Kernel_Interrupt kernel_interrupt;
    t_PID pid;
    t_TID tid;
    int status;
    bool next_cycle;

    while(1) {
        next_cycle = false;

        if(receive_kernel_interrupt(&kernel_interrupt, &pid, &tid, CLIENT_KERNEL_CPU_INTERRUPT.socket_client.fd)) {
            log_error(MODULE_LOGGER, "[%d]: Error al recibir interrupción de [Cliente] %s", CLIENT_KERNEL_CPU_INTERRUPT.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_INTERRUPT.client_type]);
            exit_sigint();
        }
        log_trace(MODULE_LOGGER, "[%d]: Se recibe interrupción de [Cliente] %s [PID: %u - TID: %u - Interrupción: %s]", CLIENT_KERNEL_CPU_INTERRUPT.socket_client.fd, PORT_NAMES[CLIENT_KERNEL_CPU_INTERRUPT.client_type], pid, tid, KERNEL_INTERRUPT_NAMES[kernel_interrupt]);

        if((status = pthread_mutex_lock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_lock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_EXECUTING);
            if(!EXECUTING) {
                next_cycle = true;
            }
        pthread_cleanup_pop(0); // MUTEX_EXECUTING
        if((status = pthread_mutex_unlock(&MUTEX_EXECUTING))) {
            log_error_pthread_mutex_unlock(status);
            exit_sigint();
        }

        if(next_cycle) {
            continue;
        }

        if((status = pthread_mutex_lock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_lock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_EXEC_CONTEXT);
            if(pid != PID && tid != TID) {
                next_cycle = true;
            }
        pthread_cleanup_pop(0); // MUTEX_EXEC_CONTEXT
        if((status = pthread_mutex_unlock(&MUTEX_EXEC_CONTEXT))) {
            log_error_pthread_mutex_unlock(status);
            exit_sigint();
        }

        if(next_cycle) {
            continue;
        }

        log_info(MINIMAL_LOGGER, "## Llega interrupción al puerto Interrupt");

        if((status = pthread_mutex_lock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_lock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_KERNEL_INTERRUPT);
            // Una forma de establecer prioridad entre interrupciones que se pisan, sólo va a quedar una
            if(KERNEL_INTERRUPT < kernel_interrupt)
                KERNEL_INTERRUPT = kernel_interrupt;
        pthread_cleanup_pop(0); // MUTEX_KERNEL_INTERRUPT
        if((status = pthread_mutex_unlock(&MUTEX_KERNEL_INTERRUPT))) {
            log_error_pthread_mutex_unlock(status);
            exit_sigint();
        }

    }

    return NULL;
}

void cpu_fetch_next_instruction(char **line) {

    if(send_instruction_request(PID, TID, EXEC_CONTEXT.cpu_registers.PC, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error(MODULE_LOGGER, "[%d]: Error al enviar solicitud de instrucción a [Servidor] %s [PID: %u - TID: %u - PC: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d]: Se envía solicitud de instrucción a [Servidor] %s [PID: %u - TID: %u - PC: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC);

    if(receive_text_with_expected_header(INSTRUCTION_REQUEST_HEADER, line, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error(MODULE_LOGGER, "[%d]: Error al recibir instrucción de [Servidor] %s [PID: %u - TID: %u - PC: %u]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d]: Se recibe instrucción de [Servidor] %s [PID: %u - TID: %u - PC: %u - Instrucción: %s]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, EXEC_CONTEXT.cpu_registers.PC, *line);
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
    if((physical_address + bytes) >= (EXEC_CONTEXT.base + EXEC_CONTEXT.limit)) {
        log_warning(MODULE_LOGGER, "mmu: %s", strerror(EFAULT));
        errno = EFAULT;
        return -1;
    }

    // Asigno la dirección física al puntero de destino
    *destination = physical_address;

    return 0;
}

void request_memory_write(size_t physical_address, void *source, size_t bytes) {
    if(source == NULL) {
        log_error(MODULE_LOGGER, "request_memory_write: %s", strerror(EINVAL));
        exit_sigint();
    }

    char *data_string = mem_hexstring(source, bytes);
    pthread_cleanup_push((void (*)(void *)) free, data_string);

        if(send_write_request(PID, TID, physical_address, source, bytes, CONNECTION_MEMORY.socket_connection.fd)) {
            log_error(MODULE_LOGGER, 
            "[%d] Error al enviar solicitud de escritura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
            "%s"
            , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes
            , data_string
            );
            exit_sigint();
        }
        log_trace(MODULE_LOGGER,
          "[%d] Se envía solicitud de escritura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
          "%s"
          , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes
          , data_string
        );

    pthread_cleanup_pop(1); // data_string

    if(receive_expected_header(WRITE_REQUEST_HEADER, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error(MODULE_LOGGER, "[%d] Error al recibir confirmación de escritura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se recibe confirmación de escritura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);
}

void request_memory_read(size_t physical_address, void *destination, size_t bytes) {
    if(destination == NULL) {
        log_error(MODULE_LOGGER, "request_memory_read: %s", strerror(EINVAL));
        exit_sigint();
    }

    if(send_read_request(PID, TID, physical_address, bytes, CONNECTION_MEMORY.socket_connection.fd)) {
        log_error(MODULE_LOGGER, "[%d] Error al enviar solicitud de lectura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se envía solicitud de lectura en espacio de usuario a [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);

    void *buffer = NULL;
    size_t bufferSize = 0;
    pthread_cleanup_push((void (*)(void *)) free, buffer);

        if(receive_data_with_expected_header(READ_REQUEST_HEADER, &buffer, &bufferSize, CONNECTION_MEMORY.socket_connection.fd)) {
            log_error(MODULE_LOGGER, "[%d] Error al recibir resultado de lectura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes);
            exit_sigint();
        }
        char *data_string = mem_hexstring(buffer, bufferSize);
        pthread_cleanup_push((void (*)(void *)) free, data_string);
            log_trace(MODULE_LOGGER,
            "[%d] Se recibe resultado de lectura en espacio de usuario de [Servidor] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
            "%s"
            , CONNECTION_MEMORY.socket_connection.fd, PORT_NAMES[CONNECTION_MEMORY.server_type], PID, TID, physical_address, bytes
            , data_string
            );
        pthread_cleanup_pop(1); // data_string

        /*
        if(bufferSize != bytes) {
            log_error(MODULE_LOGGER, "request_memory_read: No coinciden los bytes leidos (%zd) con los que se esperaban leer (%zd)", bufferSize, bytes);
            errno = EIO;
            free(buffer);
            return -1;
        }
        */

        memcpy(destination, buffer, bytes);
    
    pthread_cleanup_pop(1); // buffer
}