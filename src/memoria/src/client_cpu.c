
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "client_cpu.h"

void listen_cpu(void) {
    t_Package *package;

    while(1) {

        package = package_create();
        if(package == NULL) {
           exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) package_destroy, package);

        if(package_receive(package, CLIENT_CPU->socket_client.fd)) {
            log_error_r(&MODULE_LOGGER, "[%d] Error al recibir paquete de [Cliente] %s", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type]);
            exit_sigint();
        }
        log_trace_r(&MODULE_LOGGER, "[%d] Se recibe paquete de [Cliente] %s", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type]);

        switch(package->header) {
            case EXEC_CONTEXT_REQUEST_HEADER:
                seek_cpu_context(&(package->payload));
                break;

            case EXEC_CONTEXT_UPDATE_HEADER:
                update_cpu_context(&(package->payload));
                break;

            case INSTRUCTION_REQUEST_HEADER:
                seek_instruccion(&(package->payload));
                break;

            case READ_REQUEST_HEADER:
                read_memory(&(package->payload));
                break;

            case WRITE_REQUEST_HEADER:
                write_memory(&(package->payload));
                break;

            default:
                log_error_r(&MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
                break;
        }

        pthread_cleanup_pop(1); // package_destroy
    }
}

void seek_cpu_context(t_Payload *payload) {
    int result = 0, status;

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;

    if(payload_remove(payload, &pid, sizeof(t_PID))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(t_TID))) {
        exit_sigint();
    }

    log_trace_r(&MODULE_LOGGER, "[%d] Se recibe solicitud de contexto de ejecución de [Cliente] %s [PID: %u - TID: %u]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid);
    log_info_r(&MINIMAL_LOGGER, "## Contexto Solicitado - (PID:TID) - (%u:%u)", pid, tid);

    t_Exec_Context context;

    if((status = pthread_rwlock_rdlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_rdlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning_r(&MODULE_LOGGER, "[%d] No existe el proceso (%u)", CLIENT_CPU->socket_client.fd, pid);
            result = -1;
            goto cleanup_rwlock_proceses_and_partitions;
        }

        if((status = pthread_rwlock_rdlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_rdlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads));

            if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
                log_warning_r(&MODULE_LOGGER, "[%d] No existe el hilo (%u:%u)", CLIENT_CPU->socket_client.fd, pid, tid);
                result = -1;
                goto cleanup_rwlock_array_memory_threads;
            }

            context.cpu_registers = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers;

        cleanup_rwlock_array_memory_threads:
        pthread_cleanup_pop(0); // rwlock_array_memory_threads
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

        if(result) {
            goto cleanup_rwlock_proceses_and_partitions;
        }

        if((status = pthread_rwlock_rdlock(&(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition)))) {
            report_error_pthread_rwlock_rdlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition));
            context.base = ARRAY_PROCESS_MEMORY[pid]->partition->base;
            context.limit = ARRAY_PROCESS_MEMORY[pid]->partition->size;
        pthread_cleanup_pop(0); // rwlock_partition
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

    cleanup_rwlock_proceses_and_partitions:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_unlock(status);
        exit_sigint();
    }

    if(send_result_with_header(EXEC_CONTEXT_REQUEST_HEADER, result, CLIENT_CPU->socket_client.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d] Error al enviar resultado de solicitud de contexto de ejecución a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]" , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, result);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d] Se envía resultado de solicitud de contexto de ejecución a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, result);

    if(result) {
        return;
    }

    if(send_exec_context(context, CLIENT_CPU->socket_client.fd)) {
        log_error_r(&MODULE_LOGGER,
          "[%d] Error al enviar contexto de ejecución a [Cliente] %s [PID: %u - TID: %u]\n"
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
          , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid
          , context.cpu_registers.PC
          , context.cpu_registers.AX
          , context.cpu_registers.BX
          , context.cpu_registers.CX
          , context.cpu_registers.DX
          , context.cpu_registers.EX
          , context.cpu_registers.FX
          , context.cpu_registers.GX
          , context.cpu_registers.HX
          , context.base
          , context.limit
        );
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER,
      "[%d] Se envía contexto de ejecución a [Cliente] %s [PID: %u - TID: %u]\n"
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
      , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid
      , context.cpu_registers.PC
      , context.cpu_registers.AX
      , context.cpu_registers.BX
      , context.cpu_registers.CX
      , context.cpu_registers.DX
      , context.cpu_registers.EX
      , context.cpu_registers.FX
      , context.cpu_registers.GX
      , context.cpu_registers.HX
      , context.base
      , context.limit
    );
}

void update_cpu_context(t_Payload *payload) {
    int result = 0, status;

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;
    t_Exec_Context context;

    if(payload_remove(payload, &pid, sizeof(t_PID))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(t_TID))) {
        exit_sigint();
    }
    if(exec_context_deserialize(payload, &context)) {
        exit_sigint();
    }

    log_trace_r(&MODULE_LOGGER,
      "[%d] Se recibe actualización de contexto de ejecución de [Cliente] %s [PID: %u - TID: %u]\n"
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
      , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid
      , context.cpu_registers.PC
      , context.cpu_registers.AX
      , context.cpu_registers.BX
      , context.cpu_registers.CX
      , context.cpu_registers.DX
      , context.cpu_registers.EX
      , context.cpu_registers.FX
      , context.cpu_registers.GX
      , context.cpu_registers.HX
      , context.base
      , context.limit
    );

    if((status = pthread_rwlock_rdlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_rdlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning_r(&MODULE_LOGGER, "[%d] No existe el proceso (%u)", CLIENT_CPU->socket_client.fd, pid);
            goto cleanup_rwlock_partitions_and_processes;
        }

        if((status = pthread_rwlock_rdlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_rdlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads));

            if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
                log_warning_r(&MODULE_LOGGER, "[%d] No existe el hilo (%u:%u)", CLIENT_CPU->socket_client.fd, pid, tid);
                result = -1;
                goto cleanup_rwlock_array_memory_threads;
            }

            ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers = context.cpu_registers;

        cleanup_rwlock_array_memory_threads:
        pthread_cleanup_pop(0); // rwlock_array_memory_threads
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

    cleanup_rwlock_partitions_and_processes:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_unlock(status);
        exit_sigint();
    }

    if(send_result_with_header(EXEC_CONTEXT_UPDATE_HEADER, result, CLIENT_CPU->socket_client.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d] Error al enviar resultado de actualización de contexto de ejecución a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, result);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d] Se envía resultado de actualización de contexto de ejecución a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, result);

    log_info_r(&MINIMAL_LOGGER, "## Contexto Actualizado - (PID:TID) - (%u:%u)", pid, tid);
}

void seek_instruccion(t_Payload *payload) {
    int status;

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;
    t_PC pc;

    if(payload_remove(payload, &pid, sizeof(pid))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(tid))) {
        exit_sigint();
    }
    if(payload_remove(payload, &pc, sizeof(pc))) {
        exit_sigint();
    }

    log_trace_r(&MODULE_LOGGER, "[%d] Se recibe solicitud de instrucción de [Cliente] %s [PID: %u - TID: %u - PC: %u]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, pc);

    char *instruction = NULL;

    if((status = pthread_rwlock_rdlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_rdlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning_r(&MODULE_LOGGER, "[%d] No existe el proceso (%u)", CLIENT_CPU->socket_client.fd, pid);
            instruction = NULL;
            goto cleanup_rwlock_partitions_and_processes;
        }

        if((status = pthread_rwlock_rdlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_rdlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads));

            if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
                log_warning_r(&MODULE_LOGGER, "[%d] No existe el hilo (%u:%u)", CLIENT_CPU->socket_client.fd, pid, tid);
                instruction = NULL;
                goto cleanup_rwlock_array_memory_threads;
            }

            if(pc >= (ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count)) {
                log_warning_r(&MODULE_LOGGER, "[%d] No existe la instrucción %u del hilo (%u:%u)", CLIENT_CPU->socket_client.fd, pc, pid, tid);
                instruction = NULL;
                goto cleanup_rwlock_array_memory_threads;
            }

            instruction = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[pc];

        cleanup_rwlock_array_memory_threads:
        pthread_cleanup_pop(0); // rwlock_array_memory_threads
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

    cleanup_rwlock_partitions_and_processes:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_unlock(status);
        exit_sigint();
    }

    if(send_text_with_header(INSTRUCTION_REQUEST_HEADER, instruction, CLIENT_CPU->socket_client.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d] Error al enviar instrucción a [Cliente] %s [PID: %u - TID: %u - PC: %u - Instrucción: %s]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, pc, instruction);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d] Se envía instrucción a [Cliente] %s [PID: %u - TID: %u - PC: %u - Instrucción: %s]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, pc, instruction);

    log_info_r(&MINIMAL_LOGGER, "## Obtener instruccion - (PID:TID) - (%u:%u) - Instruccion: %s", pid, tid, instruction);
}

void read_memory(t_Payload *payload) {
    int result = 0, status;

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;
    size_t physical_address;
    size_t bytes;

    if(payload_remove(payload, &pid, sizeof(pid))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(tid))) {
        exit_sigint();
    }
    if(size_deserialize(payload, &physical_address)) {
        exit_sigint();
    }
    if(size_deserialize(payload, &bytes)) {
        exit_sigint();
    }

    log_trace_r(&MODULE_LOGGER, "[%d] Se recibe solicitud de lectura en espacio de usuario de [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes);

    if((status = pthread_rwlock_rdlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_rdlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning_r(&MODULE_LOGGER, "[%d] No existe el proceso (%u)", CLIENT_CPU->socket_client.fd, pid);
            result = -1;
            goto cleanup_rwlock_partitions_and_processes;
        }

        if((status = pthread_rwlock_rdlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_rdlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads));

            if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
                log_warning_r(&MODULE_LOGGER, "[%d] No existe el hilo (%u:%u)", CLIENT_CPU->socket_client.fd, pid, tid);
                result = -1;
                goto cleanup_rwlock_array_memory_threads;
            }

        cleanup_rwlock_array_memory_threads:
        pthread_cleanup_pop(0); // rwlock_array_memory_threads
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

        if(result) {
            goto cleanup_rwlock_partitions_and_processes;
        }

        if((status = pthread_rwlock_rdlock(&(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition)))) {
            report_error_pthread_rwlock_rdlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition));

            char *data_string = mem_hexstring((void *)(((uint8_t *) MAIN_MEMORY) + physical_address), bytes);
            pthread_cleanup_push((void (*)(void *)) free, data_string);

                if(send_data_with_header(READ_REQUEST_HEADER, (void *)(((uint8_t *) MAIN_MEMORY) + physical_address), bytes, CLIENT_CPU->socket_client.fd)) {
                    log_error_r(&MODULE_LOGGER,
                    "[%d] Error al enviar resultado de lectura en espacio de usuario a [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
                    "%s"
                    , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes
                    , data_string
                    );
                    exit_sigint();
                }
                log_trace_r(&MODULE_LOGGER,
                "[%d] Se envía resultado de lectura en espacio de usuario a [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
                "%s"
                , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes
                , data_string
                );

            pthread_cleanup_pop(1); // data_string

            log_info_r(&MINIMAL_LOGGER, "## Lectura - (PID:TID) - (%u:%u) - Dir. Fisica: %zu - Tamaño: %zu", pid, tid, physical_address, bytes);

        pthread_cleanup_pop(0); // rwlock_partition
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

    cleanup_rwlock_partitions_and_processes:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_unlock(status);
        exit_sigint();
    }

    if(result) {
        if(send_data_with_header(READ_REQUEST_HEADER, NULL, 0, CLIENT_CPU->socket_client.fd)) {
            log_error_r(&MODULE_LOGGER,
              "[%d] Error al enviar resultado de lectura en espacio de usuario a [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
              "(nil)"
              , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, (size_t) 0
            );
            exit_sigint();
        }
        log_trace_r(&MODULE_LOGGER,
          "[%d] Se envía resultado de lectura en espacio de usuario a [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
          "(nil)",
          CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, (size_t) 0
        );
    }

    log_info_r(&MINIMAL_LOGGER, "## Lectura - (PID:TID) - (%u:%u) - Dir. Fisica: %zu - Tamaño: %zu", pid, tid, physical_address, (size_t) 0);
}

void write_memory(t_Payload *payload) {
    int result = 0, status;

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;
    size_t physical_address;
    void *data;
    size_t bytes;

    if(payload_remove(payload, &pid, sizeof(pid))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(tid))) {
        exit_sigint();
    }
    if(size_deserialize(payload, &physical_address)) {
        exit_sigint();
    }
    if(data_deserialize(payload, &data, &bytes)) {
        exit_sigint();
    }

    char *data_string = mem_hexstring((void *)(((uint8_t *) MAIN_MEMORY) + physical_address), bytes);
    pthread_cleanup_push((void (*)(void *)) free, data_string);
        log_trace_r(&MODULE_LOGGER,
          "[%d] Se recibe solicitud de escritura en espacio de usuario de [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]\n"
          "%s"
          , CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes
          , data_string
        );
    pthread_cleanup_pop(1); // data_string

    if((status = pthread_rwlock_rdlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_rdlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning_r(&MODULE_LOGGER, "[%d] No existe el proceso (%u)", CLIENT_CPU->socket_client.fd, pid);
            result = -1;
            goto cleanup_rwlock_partitions_and_processes;
        }

        if((status = pthread_rwlock_rdlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_rdlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads));

            if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
                log_warning_r(&MODULE_LOGGER, "[%d] No existe el hilo (%u:%u)", CLIENT_CPU->socket_client.fd, pid, tid);
                result = -1;
                goto cleanup_rwlock_array_memory_threads;
            }

        cleanup_rwlock_array_memory_threads:
        pthread_cleanup_pop(0); // rwlock_array_memory_threads
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

        if(result) {
            goto cleanup_rwlock_partitions_and_processes;
        }

        if((status = pthread_rwlock_wrlock(&(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition)))) {
            report_error_pthread_rwlock_wrlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition));
            memcpy((void *)(((uint8_t *) MAIN_MEMORY) + physical_address), data, bytes);
        pthread_cleanup_pop(0); // rwlock_partition
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->partition->rwlock_partition)))) {
            report_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

    cleanup_rwlock_partitions_and_processes:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        report_error_pthread_rwlock_unlock(status);
        exit_sigint();
    }

    if(send_result_with_header(WRITE_REQUEST_HEADER, result, CLIENT_CPU->socket_client.fd)) {
        log_error_r(&MODULE_LOGGER, "[%d] Error al enviar resultado de escritura en espacio de usuario a [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes);
        exit_sigint();
    }
    log_trace_r(&MODULE_LOGGER, "[%d] Se envía resultado de escritura en espacio de usuario a [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CLIENT_CPU->socket_client.fd, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes);

    log_info_r(&MINIMAL_LOGGER, "## Escritura - (PID:TID) - (%u:%u) - Dir. Fisica: %zu> - Tamaño: %zu", pid, tid, physical_address, bytes);
}