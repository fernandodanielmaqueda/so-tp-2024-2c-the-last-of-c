
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "client_cpu.h"

void listen_cpu(void) {
    int result = 0, status;

    t_Package *package;

    while(1) {

        package = package_create();
        if(package == NULL) {
           exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) package_destroy, package);

        if(package_receive(package, CLIENT_CPU->fd_client)) {
            log_error(MODULE_LOGGER, "[%d] Error al recibir paquete de [Cliente] %s", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type]);
            exit_sigint();
        }
        log_trace(MODULE_LOGGER, "[%d] Se recibe paquete de [Cliente] %s", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type]);

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
                log_error(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
                break;
        }

        pthread_cleanup_pop(1); // package_destroy
    }
}

void seek_cpu_context(t_Payload *payload) {
    int result = 0;

    usleep(RESPONSE_DELAY * 1000);

    t_PID pid;
    t_TID tid;

    if(payload_remove(payload, &pid, sizeof(t_PID))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(t_TID))) {
        exit_sigint();
    }

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de contexto de ejecución de [Cliente] %s [PID: %u - TID: %u]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid);

    if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
        result = -1;
        // TODO: Responder a CPU
    }

    if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
        log_error(MODULE_LOGGER, "No se pudo encontrar el hilo %u:%u", pid, tid);
        result = -1;
        // TODO: Responder a CPU
    }

    t_Exec_Context context;
    context.cpu_registers = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers;
    context.base = ARRAY_PROCESS_MEMORY[pid]->partition->base;
    context.limit = ARRAY_PROCESS_MEMORY[pid]->size;

    if(send_exec_context(context, CLIENT_CPU->fd_client)) {
        log_error(MODULE_LOGGER,
          "[%d] Error al enviar contexto de ejecución a [Cliente] %s [PID: %u - TID: %u]"
          , CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid
        );
        exit_sigint();
    }
    log_trace(MODULE_LOGGER,
      "[%d] Se envía contexto de ejecución a [Cliente] %s [PID: %u - TID: %u]"
      , CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid
    );
    
    log_info(MINIMAL_LOGGER, "## Contexto Solicitado - (PID:TID) - (%u:%u)", pid, tid);
}

void update_cpu_context(t_Payload *payload) {

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

    log_info(MODULE_LOGGER, "[%d] Se recibe actualización de contexto de ejecución de [Cliente] %s [PID: %u - TID: %u]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid);

    if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] == NULL) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo encontrar el hilo (PID:TID): (%u:%u)", pid, tid);
        // TODO: Responder a CPU
    }

    ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->registers = context.cpu_registers;
    
    log_info(MINIMAL_LOGGER, "## Contexto Actualizado - (PID:TID) - (%u:%u)", pid, tid);
}

void seek_instruccion(t_Payload *payload) {

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

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de instrucción de [Cliente] %s [PID: %u - TID: %u - PC: %u]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, pc);

    if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
        log_warning(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
        // TODO: Responder a CPU
    }

    if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
        log_warning(MODULE_LOGGER, "No se pudo encontrar el hilo %u:%u", pid, tid);
        // TODO: Responder a CPU
    }

    if(pc >= (ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count)) {
        log_warning(MODULE_LOGGER, "[ERROR] El ProgramCounter supera la cantidad de instrucciones para el hilo (PID:TID): (%u:%u)", pid, tid);
        // TODO: Responder a CPU
    }

    char *instruction = ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[pc];

    if(send_text_with_header(INSTRUCTION_REQUEST_HEADER, instruction, CLIENT_CPU->fd_client)) {
        log_debug(MODULE_LOGGER, "[ERROR] No se pudo enviar la instruccion del proceso %d", pid);
        exit_sigint();
    }

    log_info(MINIMAL_LOGGER, "## Obtener instruccion - (PID:TID) - (%u:%u) - Instruccion: %s", pid, tid, instruction);
}

int read_memory(t_Payload *payload) {

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

    log_info(MODULE_LOGGER, "[%d] Se recibe lectura en espacio de usuario de [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes);

    // TODO

    log_info(MINIMAL_LOGGER, "## Lectura - (PID:TID) - (%u:%u) - Dir. Fisica: %zu - Tamaño: %zu", pid, tid, physical_address, bytes);

    return 0;

}

int write_memory(t_Payload *payload) {

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

    log_info(MODULE_LOGGER, "[%d] Se recibe escritura en espacio de usuario de [Cliente] %s [PID: %u - TID: %u - Dirección física: %zu - Tamaño: %zu]", CLIENT_CPU->fd_client, PORT_NAMES[CLIENT_CPU->client_type], pid, tid, physical_address, bytes);
    
    // TODO

    if(send_header(WRITE_REQUEST_HEADER, CLIENT_CPU->fd_client)) {
        // TODO
        return -1;
    }

    log_info(MINIMAL_LOGGER, "## Escritura - (PID:TID) - (%u:%u) - Dir. Fisica: %zu> - Tamaño: %zu", pid, tid, physical_address, bytes);

    return 0;
}