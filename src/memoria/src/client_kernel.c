
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "client_kernel.h"

void listen_kernel(int fd_client) {

    t_Package* package = package_create();
    if(package == NULL) {
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) package_destroy, package);

    if(package_receive(package, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al recibir paquete de [Cliente] %s", fd_client, PORT_NAMES[KERNEL_PORT_TYPE]);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se recibe paquete de [Cliente] %s", fd_client, PORT_NAMES[KERNEL_PORT_TYPE]);

    switch(package->header) {

        case PROCESS_CREATE_HEADER:
            attend_process_create(fd_client, &(package->payload));
            break;

        case PROCESS_DESTROY_HEADER:
            attend_process_destroy(fd_client, &(package->payload));
            break;
        
        case THREAD_CREATE_HEADER:
            attend_thread_create(fd_client, &(package->payload));
            break;
            
        case THREAD_DESTROY_HEADER:
            attend_thread_destroy(fd_client, &(package->payload));
            break;
            
        case MEMORY_DUMP_HEADER:
            attend_memory_dump(fd_client, &(package->payload));
            break;

        default:
            log_warning(MODULE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
            break;

    }

    pthread_cleanup_pop(1); // package_destroy
}

void attend_process_create(int fd_client, t_Payload *payload) {

    int result = 0, status;

    t_PID pid;
    size_t size;

    if(payload_remove(payload, &pid, sizeof(pid))) {
        exit_sigint();
    }
    if(size_deserialize(payload, &size)) {
        exit_sigint();
    }

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de creación de proceso de [Cliente] %s [PID: %u - Tamaño: %zu]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, size);

    // TODO: ¿Y si el proceso ya existe? ¿Lo piso? ¿Lo ignoro? ¿Termino el programa?

    t_Partition *partition = NULL;

    t_Memory_Process *new_process = malloc(sizeof(t_Memory_Process));
    if(new_process == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo proceso.", sizeof(t_Memory_Process));
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) free, new_process);

    new_process->pid = pid;
    new_process->size = size;
    new_process->tid_count = 0;
    new_process->array_memory_threads = NULL;

    if((status = pthread_rwlock_init(&(new_process->rwlock_array_memory_threads), NULL))) {
        log_error_pthread_rwlock_init(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, &(new_process->rwlock_array_memory_threads));

    if((status = pthread_rwlock_wrlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_wrlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

    // Asignar partición
    allocate_partition(&partition, new_process->size);

    if(partition == NULL) {
        log_warning(MODULE_LOGGER, "[%d] No hay particiones disponibles para la solicitud de creación de proceso %u", fd_client, new_process->pid);
        result = -1;
        goto cleanup_rwlock_proceses_and_partitions;
    }

    log_debug(MODULE_LOGGER, "[%d] Particion asignada para el pedido del proceso %u", fd_client, new_process->pid);
    partition->occupied = true;
    partition->pid = pid;
    new_process->partition = partition;

    if(add_element_to_array_process(new_process)) {
        log_debug(MODULE_LOGGER, "[%d] No se pudo agregar nuevo proceso al listado para el pedido del proceso %d", fd_client, new_process->pid);
        exit_sigint();
    }

    log_info(MINIMAL_LOGGER, "## Proceso Creado - PID: %u - TAMAÑO: %zu", new_process->pid, new_process->size);

    cleanup_rwlock_proceses_and_partitions:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_unlock(status);
    }

    pthread_cleanup_pop(0); // rwlock_array_memory_threads
    if(partition == NULL) {
        if((status = pthread_rwlock_destroy(&(new_process->rwlock_array_memory_threads)))) {
            log_error_pthread_rwlock_destroy(status);
        }
    }

    pthread_cleanup_pop(result); // new_process

    if(send_result_with_header(PROCESS_CREATE_HEADER, result, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al enviar resultado de creación de proceso a [Cliente] %s [PID: %u - Resultado: %d]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, result);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se envía resultado de creacion de proceso a [Cliente] %s [PID: %u - Resultado: %d]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, result);

}

void attend_process_destroy(int fd_client, t_Payload *payload) {

    int status;

    t_PID pid;

    if(payload_remove(payload, &pid, sizeof(pid))) {
        exit_sigint();
    }

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de finalización de proceso de [Cliente] %s [PID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid);

    size_t size;
    t_Memory_Process *process = NULL;

    if((status = pthread_rwlock_wrlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_wrlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
            goto cleanup_rwlock_proceses_and_partitions;
        }

        process = ARRAY_PROCESS_MEMORY[pid];
        ARRAY_PROCESS_MEMORY[pid] = NULL;
        process->partition->occupied = false;
        size = process->size;

        if(MEMORY_MANAGEMENT_SCHEME == DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME)
            if(verify_and_join_splited_partitions(process->partition)) {
                exit_sigint();
            }

    cleanup_rwlock_proceses_and_partitions:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_unlock(status);
        exit_sigint();
    }

    if(process_destroy(process)) {
        exit_sigint();
    }

    log_info(MINIMAL_LOGGER, "## Proceso Destruido - PID: %u - TAMAÑO: %zu", pid, size);

    if(send_header(PROCESS_DESTROY_HEADER, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al enviar confirmación de finalización de proceso a [Cliente] %s [PID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se envía confirmación de finalización de proceso a [Cliente] %s [PID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid);
}

void attend_thread_create(int fd_client, t_Payload *payload) {
    int result = 0, status;

    // TODO: ¿Y si el hilo ya existe? ¿Lo piso? ¿Lo ignoro? ¿Termino el programa?

    t_PID pid;
    t_TID tid;
    char *argument_path;

    if(payload_remove(payload, &pid, sizeof(pid))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(tid))) {
        exit_sigint();
    }
    if(text_deserialize(payload, &argument_path)) {
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) free, argument_path);

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de creación de hilo de [Cliente] %s [PID: %u - TID: %u - Archivo: %s]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid, argument_path);

    t_Memory_Thread *new_thread = malloc(sizeof(t_Memory_Thread));
    if(new_thread == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el nuevo thread.", sizeof(t_Memory_Thread));
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) free, new_thread);

    new_thread->tid = tid;
    new_thread->instructions_count = 0;
    new_thread->array_instructions = NULL;

    //Inicializar registros
    new_thread->registers.PC = 0;
    new_thread->registers.AX = 0;
    new_thread->registers.BX = 0;
    new_thread->registers.CX = 0;
    new_thread->registers.DX = 0;
    new_thread->registers.EX = 0;
    new_thread->registers.FX = 0;
    new_thread->registers.GX = 0;
    new_thread->registers.HX = 0;

    // Genera la ruta hacia el archivo de pseudocódigo
    char *target_path;
    // Ruta relativa
    target_path = malloc((INSTRUCTIONS_PATH[0] ? (strlen(INSTRUCTIONS_PATH) + 1) : 0) + strlen(argument_path) + 1);
    if(target_path == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudo reservar %zu bytes para la ruta relativa.", (INSTRUCTIONS_PATH[0] ? (strlen(INSTRUCTIONS_PATH) + 1) : 0) + strlen(argument_path) + 1);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) free, target_path);

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

    log_debug(MODULE_LOGGER, "Ruta hacia el archivo de pseudocódigo: %s", target_path);

    // Inicializar instrucciones
    if(parse_pseudocode_file(target_path, &(new_thread->array_instructions), &(new_thread->instructions_count))) {
        // TODO
        exit_sigint();
    }
    // TODO

    if((status = pthread_rwlock_rdlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_rdlock(status);
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_PARTITIONS_AND_PROCESSES);

        if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
            log_warning(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
            result = -1;
            goto cleanup_rwlock_proceses_and_partitions;
        }

        if((status = pthread_rwlock_wrlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            log_error_pthread_rwlock_wrlock(status);
            exit_sigint();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads));

            ARRAY_PROCESS_MEMORY[pid]->array_memory_threads = realloc(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads, sizeof(t_Memory_Thread *) * ((ARRAY_PROCESS_MEMORY[pid]->tid_count) + 1)); 
            if(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads == NULL) {
                log_warning(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para el array de threads.", sizeof(t_Memory_Thread *) * ((ARRAY_PROCESS_MEMORY[pid]->tid_count) + 1));
                exit_sigint();
            }

            ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid] = new_thread;
            (ARRAY_PROCESS_MEMORY[pid]->tid_count)++;

            log_info(MINIMAL_LOGGER, "## Hilo Creado - (PID:TID) - (%u:%u)", pid, tid);

        pthread_cleanup_pop(0); // rwlock_array_memory_threads
        if((status = pthread_rwlock_unlock(&(ARRAY_PROCESS_MEMORY[pid]->rwlock_array_memory_threads)))) {
            log_error_pthread_rwlock_unlock(status);
            exit_sigint();
        }

    cleanup_rwlock_proceses_and_partitions:
    pthread_cleanup_pop(0); // RWLOCK_PARTITIONS_AND_PROCESSES
    if((status = pthread_rwlock_unlock(&RWLOCK_PARTITIONS_AND_PROCESSES))) {
        log_error_pthread_rwlock_unlock(status);
        exit_sigint();
    }

    pthread_cleanup_pop(1); // target_path
    pthread_cleanup_pop(result); // new_thread
    pthread_cleanup_pop(1); // argument_path

    if(send_result_with_header(THREAD_CREATE_HEADER, result, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al enviar resultado de creación de hilo a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid, result);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se envía resultado de creación de hilo a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid, result);

}

void attend_thread_destroy(int fd_client, t_Payload *payload) {

    t_PID pid;
    t_TID tid;

    if(payload_remove(payload, &pid, sizeof(t_PID))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(t_TID))) {
        exit_sigint();
    }

    log_info(MODULE_LOGGER, "[%d] Se recibe solicitud de finalización de hilo de [Cliente] %s [PID: %u - TID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid);

    if((pid >= PID_COUNT) || ((ARRAY_PROCESS_MEMORY[pid]) == NULL)) {
        log_warning(MODULE_LOGGER, "No se pudo encontrar el proceso %u", pid);
        goto send_result;
    }

    if((tid >= (ARRAY_PROCESS_MEMORY[pid]->tid_count)) || ((ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]) == NULL)) {
        log_warning(MODULE_LOGGER, "No se pudo encontrar el hilo %u:%u", pid, tid);
        goto send_result;
    }

    // Free instrucciones
    for(t_PC pc = 0; pc < ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->instructions_count; pc++) {
        free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions[pc]);
    }

    free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]->array_instructions);
    free(ARRAY_PROCESS_MEMORY[pid]->array_memory_threads[tid]);

    log_info(MINIMAL_LOGGER, "## Hilo Destruido - (PID:TID) - (%u:%u)", pid, tid);

    send_result:
    if(send_header(THREAD_DESTROY_HEADER, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al enviar confirmación de finalización de hilo a [Cliente] %s [PID: %u - TID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se envía confirmación de finalización de hilo a [Cliente] %s [PID: %u - TID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid);
}

void attend_memory_dump(int fd_client, t_Payload *payload) {
    int result = 0;

    t_PID pid;
    t_TID tid;

    if(payload_remove(payload, &pid, sizeof(t_PID))) {
        exit_sigint();
    }
    if(payload_remove(payload, &tid, sizeof(t_TID))) {
        exit_sigint();
    }

    log_info(MODULE_LOGGER, "[%d] Kernel: Se recibe solicitud de volcado de memoria de [Cliente] %s [PID: %u - TID: %u]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid);

    // 1-0-12:51:59:331.dmp
    char *filename = string_from_format("%u-%u-", pid, tid);
    if(filename == NULL) {
        log_error(MODULE_LOGGER, "string_from_format: No se pudo crear el nombre del archivo");
        exit_sigint();
    }
    pthread_cleanup_push((void (*)(void *)) free, filename);

        char *string_time = temporal_get_string_time("%H:%M:%S:%MS");
        if(string_time == NULL) {
            log_error(MODULE_LOGGER, "temporal_get_string_time: No se pudo obtener la hora actual como un string");
            exit_sigint();
        }        
        pthread_cleanup_push((void (*)(void *)) free, string_time);
            string_append(&filename, string_time);
        pthread_cleanup_pop(1); // string_time

        string_append(&filename, ".dmp");
        
        t_Connection connection_filesystem = (t_Connection) {.client_type = MEMORY_PORT_TYPE, .server_type = FILESYSTEM_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_FILESYSTEM"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_FILESYSTEM")};

        client_thread_connect_to_server(&connection_filesystem);
        pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_filesystem.fd_connection));

            char *dump_string = mem_hexstring((void *)(((uint8_t *) MAIN_MEMORY) + ARRAY_PROCESS_MEMORY[pid]->partition->base), ARRAY_PROCESS_MEMORY[pid]->size);
            pthread_cleanup_push((void (*)(void *)) free, dump_string);

                if(send_memory_dump(filename, (void *)(((uint8_t *) MAIN_MEMORY) + ARRAY_PROCESS_MEMORY[pid]->partition->base), ARRAY_PROCESS_MEMORY[pid]->size, connection_filesystem.fd_connection)) {
                    log_error(MODULE_LOGGER,
                      "[%d] Error al enviar operación de volcado de memoria a [Servidor] %s [PID: %u - TID: %u - Archivo: %s - Tamaño: %zu]\n"
                      "%s"
                      , connection_filesystem.fd_connection, PORT_NAMES[connection_filesystem.server_type], pid, tid, filename, ARRAY_PROCESS_MEMORY[pid]->size
                      , dump_string
                    );
                    exit_sigint();
                }
                log_trace(MODULE_LOGGER,
                  "[%d] Se envía operación de volcado de memoria a [Servidor] %s [PID: %u - TID: %u - Archivo: %s - Tamaño: %zu]\n"
                  "%s"
                  , connection_filesystem.fd_connection, PORT_NAMES[connection_filesystem.server_type], pid, tid, filename, ARRAY_PROCESS_MEMORY[pid]->size
                  , dump_string
                );

            pthread_cleanup_pop(1); // dump_string

            log_info(MINIMAL_LOGGER, "## Memory Dump solicitado - (PID:TID) - (%u:%u)", pid, tid);

            if(receive_result_with_expected_header(MEMORY_DUMP_HEADER, &result, connection_filesystem.fd_connection)) {
                log_error(MODULE_LOGGER, "[%d] Error al recibir resultado de operación de volcado de memoria de [Servidor] %s [PID: %u - TID: %u]", connection_filesystem.fd_connection, PORT_NAMES[connection_filesystem.server_type], pid, tid);
                exit_sigint();
            }
            log_trace(MODULE_LOGGER, "[%d] Se recibe resultado de operación de volcado de memoria de [Servidor] %s [PID: %u - TID: %u - Resultado: %d]", connection_filesystem.fd_connection, PORT_NAMES[connection_filesystem.server_type], pid, tid, result);

        pthread_cleanup_pop(0);
        if(close(connection_filesystem.fd_connection)) {
            log_error_close();
            exit_sigint();
        }

    pthread_cleanup_pop(1); // filename

    if(send_result_with_header(MEMORY_DUMP_HEADER, result, fd_client)) {
        log_error(MODULE_LOGGER, "[%d] Error al enviar resultado de volcado de memoria a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid, result);
        exit_sigint();
    }
    log_trace(MODULE_LOGGER, "[%d] Se envía resultado de volcado de memoria a [Cliente] %s [PID: %u - TID: %u - Resultado: %d]", fd_client, PORT_NAMES[KERNEL_PORT_TYPE], pid, tid, result);
}

void allocate_partition(t_Partition **partition, size_t required_size) {
    if(partition == NULL) {
        exit_sigint();
    }

    *partition = NULL;
    size_t index_partition;
    t_Partition *aux_partition;

    switch(MEMORY_ALLOCATION_ALGORITHM) {
        case FIRST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((!(aux_partition->occupied)) && ((aux_partition->size) >= required_size)) {
                    *partition = aux_partition;
                    index_partition = i;
                    break;
                }
            }

            break;
        }

        case BEST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((!(aux_partition->occupied)) && ((aux_partition->size) >= required_size)) {
                    if((*partition) == NULL) {
                        *partition = aux_partition;
                        index_partition = i;
                    }
                    else {
                        if((aux_partition->size) < ((*partition)->size)) {
                            *partition = aux_partition;
                            index_partition = i;
                        }
                    }
                }
            }

            break;
        }

        case WORST_FIT_MEMORY_ALLOCATION_ALGORITHM: {

            for(size_t i = 0; i < list_size(PARTITION_TABLE); i++) {
                aux_partition = list_get(PARTITION_TABLE, i);
                if((!(aux_partition->occupied)) && ((aux_partition->size) >= required_size)) {
                    if((*partition) == NULL) {
                        *partition = aux_partition;
                        index_partition = i;
                    }
                    else {
                        if((aux_partition->size) > ((*partition)->size)) {
                            *partition = aux_partition;
                            index_partition = i;
                        }
                    }
                }
            }

            break;
        }
    }

    if((*partition) != NULL) {
        // Realiza el fraccionamiento de la particion (si es requerido)
        switch(MEMORY_MANAGEMENT_SCHEME) {

            case FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME: {
                break;
            }

            case DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME: {

                if(split_partition(index_partition, required_size)) {
                    exit_sigint();
                }

                break;
            }
        }
    }
}

int split_partition(size_t index_partition, size_t required_size) {
    int retval = 0, status;

    t_Partition *old_partition = list_get(PARTITION_TABLE, index_partition);

    if(old_partition->size == required_size) {
        return 0;
    }

    t_Partition *new_partition = malloc(sizeof(t_Partition));
    if(new_partition == NULL) {
        fprintf(stderr, "malloc: No se pudieron reservar %zu bytes para una particion\n", sizeof(t_Partition));
        return -1;
    }
    pthread_cleanup_push((void (*)(void *)) free, new_partition);

        if((status = pthread_rwlock_init(&(new_partition->rwlock_partition), NULL))) {
            log_error_pthread_rwlock_init(status);
            retval = -1;
            goto cleanup_new_partition;
        }
        pthread_cleanup_push((void (*)(void *)) pthread_rwlock_destroy, (void *) &(new_partition->rwlock_partition));

            new_partition->size = (old_partition->size - required_size);
            new_partition->base = (old_partition->base + required_size);
            new_partition->occupied = false;

            old_partition->size = required_size;

            index_partition++;

            list_add_in_index(PARTITION_TABLE, index_partition, new_partition);

            pthread_cleanup_pop(0); // rwlock
            if(retval) {
                if((status = pthread_rwlock_destroy(&(new_partition->rwlock_partition)))) {
                    log_error_pthread_rwlock_destroy(status);
                }
            }

    cleanup_new_partition:
    pthread_cleanup_pop(retval);

    return retval;

}

int add_element_to_array_process(t_Memory_Process* process) {

    ARRAY_PROCESS_MEMORY = realloc(ARRAY_PROCESS_MEMORY, sizeof(t_Memory_Process *) * (PID_COUNT + 1));    
    if(ARRAY_PROCESS_MEMORY == NULL) {
        log_warning(MODULE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", sizeof(t_Memory_Process *) * PID_COUNT, sizeof(t_Memory_Process *) * (PID_COUNT +1));
        return -1;
    }

    ARRAY_PROCESS_MEMORY[PID_COUNT] = process;
    PID_COUNT++;

    return 0;
}

int verify_and_join_splited_partitions(t_Partition *partition) {

    t_link_element *current = PARTITION_TABLE->head;
    size_t i;
    for(i = 0; partition != (current->data); i++) {
        current = current->next;
    }

    // No es la primera ni la última partición
    if((i != 0) && (i != (list_size(PARTITION_TABLE) - 1))) {
        t_Partition *aux_partition_right;
        t_Partition *aux_partition_left;
    
        aux_partition_right = list_get(PARTITION_TABLE, (i + 1));
        aux_partition_left = list_get(PARTITION_TABLE, (i - 1));

        if(!(aux_partition_right->occupied)) {
            partition->size += aux_partition_right->size;
            list_remove_and_destroy_element(PARTITION_TABLE, (i + 1), (void (*)(void *)) partition_destroy);
        }

        if(!(aux_partition_left->occupied)) {
            aux_partition_left->size += partition->size;
            list_remove_and_destroy_element(PARTITION_TABLE, i, (void (*)(void *)) partition_destroy);
        }

        return 0;
    }

    // Es la primera partición (con cantidad de particiones > 1)
    if((i == 0) && (i != (list_size(PARTITION_TABLE) - 1))) {
        t_Partition *aux_partition_right;

        aux_partition_right = list_get(PARTITION_TABLE, (i + 1));

        if(!(aux_partition_right->occupied)) {
            partition->size += aux_partition_right->size;
            list_remove_and_destroy_element(PARTITION_TABLE, (i + 1), (void (*)(void *)) partition_destroy);
        }

        return 0;
    }

    // Es la última partición (con cantidad de particiones > 1)
    if((i != 0) && (i == (list_size(PARTITION_TABLE) - 1))) {
        t_Partition *aux_partition_left;

        aux_partition_left = list_get(PARTITION_TABLE, (i - 1));

        if(!(aux_partition_left->occupied)) {
            aux_partition_left->size += partition->size;
            list_remove_and_destroy_element(PARTITION_TABLE, i, (void (*)(void *)) partition_destroy);
        }

        return 0;
    }

    // Es la única partición
    return 0;
}



int parse_pseudocode_file(char *path, char ***array_instruction, t_PC *count) {
    FILE* file;
    if((file = fopen(path, "r")) == NULL) {
        log_warning(MODULE_LOGGER, "%s: No se pudo abrir el archivo de pseudocodigo indicado.", path);
        return -1;
    }
    char *line = NULL, *subline;
    size_t length;
    ssize_t nread;
    while(1) {
        errno = 0;
        if((nread = getline(&line, &length, file)) == -1) {
            if(errno) {
                log_warning(MODULE_LOGGER, "getline: %s", strerror(errno));
                free(line);
                if(fclose(file)) {
                    log_error_fclose();
                }
                exit(EXIT_FAILURE);
            }
            // Se terminó de leer el archivo
            break;
        }
        // Ignora líneas en blanco
        subline = strip_whitespaces(line);
        if(*subline) { // Se leyó una línea con contenido
    
            *array_instruction = realloc(*array_instruction, (*count + 1) * sizeof(char *));
            if(*array_instruction == NULL) {
                perror("[Error] realloc memory for array fallo.");
                free(line);
                if(fclose(file)) {
                    log_error_fclose();
                }
                exit(EXIT_FAILURE);
            }
            (*array_instruction)[*count] = strdup(subline);
            if((*array_instruction)[*count] == NULL) {
                perror("[Error] malloc memory for string fallo.");
                free(line);
                if(fclose(file)) {
                    log_error_fclose();
                }
                exit(EXIT_FAILURE);
            }
            (*count)++;
        }
    }
    free(line);       
    if(fclose(file)) {
        log_error_fclose();
        return -1;
    }
    return 0;
}

int process_destroy(t_Memory_Process *process) {
    int retval = 0, status;

    // Liberacion de threads con sus struct
    free_threads(process);

    if((status = pthread_rwlock_destroy(&(process->rwlock_array_memory_threads)))) {
        log_error_pthread_rwlock_destroy(status);
        retval = -1;
    }

    free(process);

    return retval;
}