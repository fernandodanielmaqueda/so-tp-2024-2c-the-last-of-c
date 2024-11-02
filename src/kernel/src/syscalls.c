#include "syscalls.h"

t_Syscall SYSCALLS[] = {
    [PROCESS_CREATE_CPU_OPCODE] = {.name = "PROCESS_CREATE" , .function = process_create_kernel_syscall},
    [PROCESS_EXIT_CPU_OPCODE] = {.name = "PROCESS_EXIT" , .function = process_exit_kernel_syscall},
    [THREAD_CREATE_CPU_OPCODE] = {.name = "THREAD_CREATE" , .function = thread_create_kernel_syscall},
    [THREAD_JOIN_CPU_OPCODE] = {.name = "THREAD_JOIN" , .function = thread_join_kernel_syscall},
    [THREAD_CANCEL_CPU_OPCODE] = {.name = "THREAD_CANCEL" , .function = thread_cancel_kernel_syscall},
    [THREAD_EXIT_CPU_OPCODE] = {.name = "THREAD_EXIT" , .function = thread_exit_kernel_syscall},
    [MUTEX_CREATE_CPU_OPCODE] = {.name = "MUTEX_CREATE" , .function = mutex_create_kernel_syscall},
    [MUTEX_LOCK_CPU_OPCODE] = {.name = "MUTEX_LOCK" , .function = mutex_lock_kernel_syscall},
    [MUTEX_UNLOCK_CPU_OPCODE] = {.name = "MUTEX_UNLOCK" , .function = mutex_unlock_kernel_syscall},
    [DUMP_MEMORY_CPU_OPCODE] = {.name = "DUMP_MEMORY" , .function = dump_memory_kernel_syscall},
    [IO_CPU_OPCODE] = {.name = "IO" , .function = io_kernel_syscall}
};

int syscall_execute(t_Payload *syscall_instruction) {

    e_CPU_OpCode syscall_opcode;
    if(cpu_opcode_deserialize(syscall_instruction, &syscall_opcode)) {
        error_pthread();
    }

    if(syscall_opcode >= (sizeof(SYSCALLS) / sizeof(t_Syscall))) {
        log_warning(MODULE_LOGGER, "Syscall no encontrada");
        error_pthread();
    }

    log_info(MINIMAL_LOGGER, "## (%u:%u) - Solicitó syscall: %s", TCB_EXEC->pcb->PID, TCB_EXEC->TID, SYSCALLS[syscall_opcode].name);

    if(SYSCALLS[syscall_opcode].function(syscall_instruction)) {
        return -1;
    }

    return 0;
}

int process_create_kernel_syscall(t_Payload *syscall_arguments) {

    char *pseudocode_filename;
    if(text_deserialize(syscall_arguments, &pseudocode_filename)) {
        error_pthread();
    }

    size_t size;
    if(size_deserialize(syscall_arguments, &size)) {
        error_pthread();
    }

    t_Priority priority;
    if(payload_remove(syscall_arguments, &priority, sizeof(priority))) {
        error_pthread();
    }

    log_trace(MODULE_LOGGER, "PROCESS_CREATE %s %zu %u", pseudocode_filename, size, priority);

    if(new_process(size, pseudocode_filename, priority)) {
        error_pthread();
    }

    SHOULD_REDISPATCH = 1;
    return 0;
}

int process_exit_kernel_syscall(t_Payload *syscall_arguments) {
    t_TCB *tcb;
    int status;

    log_trace(MODULE_LOGGER, "PROCESS_EXIT");

    // Cambio el rdlock por wrlock
    if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_wrlock, &SCHEDULING_RWLOCK);
    if((status = pthread_rwlock_wrlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_wrlock(status);
        error_pthread();
    }
    pthread_cleanup_pop(0); // SCHEDULING_RWLOCK

        KILL_EXIT_REASON = PROCESS_EXIT_EXIT_REASON;

        for(t_TID tid = 0; tid < TCB_EXEC->pcb->thread_manager.counter; tid++) {
            tcb = ((t_TCB **) TCB_EXEC->pcb->thread_manager.array)[tid];
            if(tcb != NULL) {
                kill_thread(tcb);
            }
        }

    // Regreso del wrlock al rdlock
    if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_rdlock, &SCHEDULING_RWLOCK);
    if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_rdlock(status);
        error_pthread();
    }
    pthread_cleanup_pop(0); // SCHEDULING_RWLOCK

    // El hilo actual ya se envió a EXIT
    SHOULD_REDISPATCH = 0;
    return 0;
}

int thread_create_kernel_syscall(t_Payload *syscall_arguments) {

    char *pseudocode_filename;
    if(text_deserialize(syscall_arguments, &pseudocode_filename)) {
        error_pthread();
    }

    t_Priority priority;
    if(payload_remove(syscall_arguments, &priority, sizeof(priority))) {
        error_pthread();
    }

    log_trace(MODULE_LOGGER, "THREAD_CREATE %s %u", pseudocode_filename, priority);

    t_TCB *new_tcb = tcb_create(TCB_EXEC->pcb, pseudocode_filename, priority);
    if(new_tcb == NULL) {
        error_pthread();
    }

    // Ya tengo rdlock de SCHEDULING_RWLOCK
    if(array_list_ready_update(new_tcb->priority)) {
        error_pthread();
    }
    log_info(MINIMAL_LOGGER, "## (%u:%u) Se crea el Hilo - Estado: READY", new_tcb->pcb->PID, new_tcb->TID);
    insert_state_ready(new_tcb);

    SHOULD_REDISPATCH = 1;
    return 0;
}

int thread_join_kernel_syscall(t_Payload *syscall_arguments) {
    int status;

    t_TID tid;
    if(payload_remove(syscall_arguments, &tid, sizeof(tid))) {
        error_pthread();
    }

    log_trace(MODULE_LOGGER, "THREAD_JOIN %u", tid);

    // Caso 1: Si se une a sí mismo (falla pero se hace redispatch)
    if(tid == TCB_EXEC->TID) {
        SHOULD_REDISPATCH = 1;
        return 0;
    }

    // Cambio el rdlock por wrlock
    if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_wrlock, &SCHEDULING_RWLOCK);
    if((status = pthread_rwlock_wrlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_wrlock(status);
        error_pthread();
    }
    pthread_cleanup_pop(0); // SCHEDULING_RWLOCK

        // Caso 2A: Si se une a otro y falla (se hace redispatch)
        if(tid >= TCB_EXEC->pcb->thread_manager.counter) {
            log_warning(MODULE_LOGGER, "No existe un hilo con TID <%u>", tid);
            SHOULD_REDISPATCH = 1;
            goto cleanup_scheduling_rwlock;
        }

        t_TCB *tcb = ((t_TCB **) TCB_EXEC->pcb->thread_manager.array)[tid];
        // Caso 2B: Si se une a otro y falla (se hace redispatch)
        if(tcb == NULL) {
            log_warning(MODULE_LOGGER, "No existe un hilo con TID <%u>", tid);
            SHOULD_REDISPATCH = 1;
            goto cleanup_scheduling_rwlock;
        }

        // Caso 3: Si se une a otro y no falla (se bloquea)
        SHOULD_REDISPATCH = 0;
        if(get_state_exec(&TCB_EXEC)) {
            error_pthread();
        }
        if(insert_state_blocked_join(TCB_EXEC, tcb)) {
            error_pthread();
        }

    cleanup_scheduling_rwlock:
    // Regreso del wrlock al rdlock
    if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_rdlock, &SCHEDULING_RWLOCK);
    if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_rdlock(status);
        error_pthread();
    }
    pthread_cleanup_pop(0); // SCHEDULING_RWLOCK

    return 0;
}

int thread_cancel_kernel_syscall(t_Payload *syscall_arguments) {
    int status;

    t_TID tid;
    if(payload_remove(syscall_arguments, &tid, sizeof(tid))) {
        error_pthread();
    }

    log_trace(MODULE_LOGGER, "THREAD_CANCEL %u", tid);

    // Caso 1: Si se cancela a sí mismo
    if(tid == TCB_EXEC->TID) {
        TCB_EXEC->exit_reason = THREAD_CANCEL_EXIT_REASON;
        return -1;
    }

    // Caso 2: Si cancela a otros (falle o no se hace redispatch)
    SHOULD_REDISPATCH = 1;

    // Cambio el rdlock por wrlock
    if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_wrlock, &SCHEDULING_RWLOCK);
    if((status = pthread_rwlock_wrlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_wrlock(status);
        error_pthread();
    }
    pthread_cleanup_pop(0); // SCHEDULING_RWLOCK

        if(tid >= TCB_EXEC->pcb->thread_manager.counter) {
            log_warning(MODULE_LOGGER, "No existe un hilo con TID <%u>", tid);
            goto cleanup_scheduling_rwlock;
        }

        KILL_EXIT_REASON = THREAD_CANCEL_EXIT_REASON;

        t_TCB *tcb = ((t_TCB **) TCB_EXEC->pcb->thread_manager.array)[tid];
        if(tcb == NULL) {
            log_warning(MODULE_LOGGER, "No existe un hilo con TID <%u>", tid);
            goto cleanup_scheduling_rwlock;
        }

        kill_thread(tcb);

    cleanup_scheduling_rwlock:
    // Regreso del wrlock al rdlock
    if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_unlock(status);
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) pthread_rwlock_rdlock, &SCHEDULING_RWLOCK);
    if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_rdlock(status);
        error_pthread();
    }
    pthread_cleanup_pop(0); // SCHEDULING_RWLOCK

    return 0;
}

int thread_exit_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_EXIT");

    TCB_EXEC->exit_reason = THREAD_EXIT_EXIT_REASON;
    return -1;
}

int mutex_create_kernel_syscall(t_Payload *syscall_arguments) {
    int retval = 0, status;

    char *resource_name;
    if(text_deserialize(syscall_arguments, &resource_name)) {
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) free, resource_name);

    log_trace(MODULE_LOGGER, "MUTEX_CREATE %s", resource_name);

	if((status = pthread_rwlock_wrlock(&(TCB_EXEC->pcb->rwlock_dictionary_mutexes)))) {
		log_error_pthread_rwlock_wrlock(status);
		error_pthread();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(TCB_EXEC->pcb->rwlock_dictionary_mutexes));

        if(dictionary_has_key(TCB_EXEC->pcb->dictionary_mutexes, resource_name)) {
            log_warning(MODULE_LOGGER, "%s: Ya existe un mutex creado con el mismo nombre", resource_name);
            TCB_EXEC->exit_reason = INVALID_RESOURCE_EXIT_REASON;
            retval = -1;
            goto cleanup;
        }

        t_Resource *resource = resource_create();
        if(resource == NULL) {
            log_error(MODULE_LOGGER, "resource_create: No se pudo crear el recurso");
            error_pthread();
        }

        dictionary_put(TCB_EXEC->pcb->dictionary_mutexes, resource_name, resource);

    cleanup:
	pthread_cleanup_pop(0); // rwlock_dictionary_mutexes
	if((status = pthread_rwlock_unlock(&(TCB_EXEC->pcb->rwlock_dictionary_mutexes)))) {
		log_error_pthread_rwlock_unlock(status);
		error_pthread();
	}
    pthread_cleanup_pop(1); // resource_name

    if(retval)
        return -1;

    SHOULD_REDISPATCH = 1;
    return 0;
}

int mutex_lock_kernel_syscall(t_Payload *syscall_arguments) {
    int retval = 0, status;

    char *resource_name;
    if(text_deserialize(syscall_arguments, &resource_name)) {
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) free, resource_name);

    log_trace(MODULE_LOGGER, "MUTEX_LOCK %s", resource_name);

	if((status = pthread_rwlock_rdlock(&(TCB_EXEC->pcb->rwlock_dictionary_mutexes)))) {
		log_error_pthread_rwlock_rdlock(status);
		error_pthread();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(TCB_EXEC->pcb->rwlock_dictionary_mutexes));

        if(dictionary_has_key(TCB_EXEC->pcb->dictionary_mutexes, resource_name)) {
            log_warning(MODULE_LOGGER, "%s: No existe un mutex creado con el nombre indicado", resource_name);
            TCB_EXEC->exit_reason = INVALID_RESOURCE_EXIT_REASON;
            retval = -1;
            goto cleanup;
        }

        t_Resource *resource = dictionary_get(TCB_EXEC->pcb->dictionary_mutexes, resource_name);
        if(resource == NULL) {
            log_error(MODULE_LOGGER, "dictionary_get: No se pudo obtener el recurso asociado al nombre");
            error_pthread();
        }

        /*
        if((status = pthread_mutex_lock(&(resource->mutex_instances)))) {
            log_error_pthread_mutex_lock(status);
            error_pthread();
        }
        pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(resource->mutex_instances));

        resource->instances--;

        if(resource->instances < 0) {
            if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
                log_error_pthread_mutex_unlock(status);
                error_pthread();
            }

            switch_state(SYSCALL_PCB, BLOCKED_MUTEX_STATE);

            if(status = pthread_mutex_lock(&(resource->shared_list_blocked.mutex))) {
                log_error_pthread_mutex_lock(status);
                error_pthread();
            }
                list_add(resource->shared_list_blocked.list, SYSCALL_PCB);
                log_info(MINIMAL_LOGGER, "PID: %d - Bloqueado por: %s", (int) SYSCALL_PCB->PID, resource_name);
                SYSCALL_PCB->location = &(resource->shared_list_blocked);
            if(status = pthread_mutex_unlock(&(resource->shared_list_blocked.mutex))) {
                log_error_pthread_mutex_unlock(status);
                error_pthread();
            }

            EXEC_PCB = 0;
        } else {
            if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
                log_error_pthread_mutex_unlock(status);
                error_pthread();
            }

            list_add(SYSCALL_PCB->assigned_resources, resource);

            EXEC_PCB = 1;
        }
        */

    cleanup:
	pthread_cleanup_pop(0); // rwlock_dictionary_mutexes
	if((status = pthread_rwlock_unlock(&(TCB_EXEC->pcb->rwlock_dictionary_mutexes)))) {
		log_error_pthread_rwlock_unlock(status);
		error_pthread();
	}
    pthread_cleanup_pop(1); // resource_name

    return 0;
}

// TODO: Validar las implementaciones de funciones de diccionarios de que...
// el diccionario contenga entradas y no tire segfault al hacer dictionary_get y dictionary_remove
// dictionary_get y dictionary_remove: posible segfault si la key no existe en el diccionario.
// dictionary_remove: se hace el free de la key al remover el elemento

int mutex_unlock_kernel_syscall(t_Payload *syscall_arguments) {
    int retval = 0, status;

    char *resource_name;
    if(text_deserialize(syscall_arguments, &resource_name)) {
        error_pthread();
    }
    pthread_cleanup_push((void (*)(void *)) free, resource_name);

    log_trace(MODULE_LOGGER, "MUTEX_UNLOCK %s", resource_name);

	if((status = pthread_rwlock_rdlock(&(TCB_EXEC->pcb->rwlock_dictionary_mutexes)))) {
		log_error_pthread_rwlock_rdlock(status);
		error_pthread();
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &(TCB_EXEC->pcb->rwlock_dictionary_mutexes));

        if(dictionary_has_key(TCB_EXEC->pcb->dictionary_mutexes, resource_name)) {
            log_warning(MODULE_LOGGER, "%s: No existe un mutex creado con el nombre indicado", resource_name);
            TCB_EXEC->exit_reason = INVALID_RESOURCE_EXIT_REASON;
            retval = -1;
            goto cleanup;
        }

        t_Resource *resource = dictionary_get(TCB_EXEC->pcb->dictionary_mutexes, resource_name);
        if(resource == NULL) {
            //log_error(MODULE_LOGGER, "mutex_lock_kernel_syscall: No se pudo obtener el recurso");
            error_pthread();
        }

        // CAMBIA ALGO: TENGO QUE VALIDAR QUE EL MUTEX ESTÉ TOMADO POR EL HILO EN EJECUCIÓN
        if(dictionary_has_key(TCB_EXEC->dictionary_assigned_mutexes, resource_name)) {
            log_warning(MODULE_LOGGER, "%s: El hilo no tiene asignado un mutex con el nombre indicado", resource_name);
            TCB_EXEC->exit_reason = INVALID_RESOURCE_EXIT_REASON;
            retval = -1;
            goto cleanup;
        }

        dictionary_remove(TCB_EXEC->dictionary_assigned_mutexes, resource_name);

        // free(resource_name);

        /*
        EXEC_PCB = 1;

        if((status = pthread_mutex_lock(&(resource->mutex_instances)))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }

        resource->instances++;

        if(resource->instances <= 0) {
            if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }

            if(status = pthread_mutex_unlock(&(resource->shared_list_blocked.mutex))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }

                if((resource->shared_list_blocked.list)->head == NULL) {
                    if(status = pthread_mutex_unlock(&(resource->shared_list_blocked.mutex))) {
                        log_error_pthread_mutex_unlock(status);
                        // TODO
                    }
                    return 0;
                }

                t_PCB *pcb = (t_PCB *) list_remove(resource->shared_list_blocked.list, 0);

            if(status = pthread_mutex_unlock(&(resource->shared_list_blocked.mutex))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }

            list_add(pcb->assigned_resources, resource);
        
            switch_state(pcb, READY_STATE);
        }
        else {
            if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
                log_error_pthread_mutex_unlock(status);
                // TODO
            }
        }
        */

    cleanup:
	pthread_cleanup_pop(0);
	if((status = pthread_rwlock_unlock(&(TCB_EXEC->pcb->rwlock_dictionary_mutexes)))) {
		log_error_pthread_rwlock_unlock(status);
		error_pthread();
	}
    pthread_cleanup_pop(0); // REVISAR

    return 0;
}

int dump_memory_kernel_syscall(t_Payload *syscall_arguments) {
    int status;

    log_trace(MODULE_LOGGER, "DUMP_MEMORY");

	t_Dump_Memory_Petition *dump_memory_petition = malloc(sizeof(t_Dump_Memory_Petition));
	if(dump_memory_petition == NULL) {
		log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zu bytes para una peticion de DUMP_MEMORY", sizeof(t_Dump_Memory_Petition));
		error_pthread();
	}
    pthread_cleanup_push((void (*)(void *)) free, dump_memory_petition);
        dump_memory_petition->bool_thread.running = false;
        dump_memory_petition->tcb = TCB_EXEC;

        if(get_state_exec(&TCB_EXEC)) {
            error_pthread();
        }
        if(insert_state_blocked_dump_memory(dump_memory_petition)) {
            error_pthread();
        }
    pthread_cleanup_pop(0);

    SHOULD_REDISPATCH = 0;
    return 0;
}

int io_kernel_syscall(t_Payload *syscall_arguments) {

    size_t previous_offset = syscall_arguments->offset;
        t_Time time;
        if(payload_read(syscall_arguments, &time, sizeof(time))) {
            error_pthread();
        }
    if(payload_seek(syscall_arguments, previous_offset, SEEK_SET)) {
        error_pthread();
    }

    log_trace(MODULE_LOGGER, "IO %lu", time);

    if(get_state_exec(&TCB_EXEC)) {
        error_pthread();
    }
    if(insert_state_blocked_io_ready(TCB_EXEC)) {
        error_pthread();
    }

    SHOULD_REDISPATCH = 0;
    return 0;
}