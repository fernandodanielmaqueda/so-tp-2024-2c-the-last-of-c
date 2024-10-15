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
    cpu_opcode_deserialize(syscall_instruction, &syscall_opcode);

    if(SYSCALLS[syscall_opcode].function == NULL) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            payload_destroy(syscall_instruction);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        log_error(MODULE_LOGGER, "Funcion de syscall no encontrada");
        return -1;
    }

    int retval = SYSCALLS[syscall_opcode].function(syscall_instruction);
    /*
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        payload_destroy(syscall_instruction);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    */
    return retval;
}

int process_create_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "PROCESS_CREATE");

    char *pseudocode_filename;
    text_deserialize(syscall_arguments, &pseudocode_filename);

    size_t size;
    size_deserialize(syscall_arguments, &size);

    t_Priority priority;
    payload_remove(syscall_arguments, &priority, sizeof(t_Priority));

    if(new_process(size, pseudocode_filename, priority)) {
        log_warning(MODULE_LOGGER, "PROCESS_CREATE: No se pudo crear el proceso");
        return -1;
    }

    SHOULD_REDISPATCH = 1;

    return 0;
}

int process_exit_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "PROCESS_EXIT");

    // TODO

    SHOULD_REDISPATCH = 0;

    return 0;
}

int thread_create_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_CREATE");

    char *pseudocode_filename;
    text_deserialize(syscall_arguments, &pseudocode_filename);

    t_Priority priority;
    payload_remove(syscall_arguments, &priority, sizeof(t_Priority));
   
    // TODO

    /*
    t_TCB *new_tcb = tcb_create(EXEC_TCB->pcb);
    if(new_tcb == NULL) {
        log_error(MODULE_LOGGER, "No se pudo crear el TCB");
        return -1;
    }
    */

    SHOULD_REDISPATCH = 1;

    return 0;
}

int thread_join_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_JOIN");

    t_TID tid;
    payload_remove(syscall_arguments, &tid, sizeof(t_TID));

    // TODO

    SHOULD_REDISPATCH = 0;

    return 0;
}

int thread_cancel_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_CANCEL");

    t_TID tid;
    payload_remove(syscall_arguments, &tid, sizeof(t_TID));

    // TODO

    // Si es suicida (se cancela a sí mismo)
    SHOULD_REDISPATCH = 0;
    // Si cancela a otros
    SHOULD_REDISPATCH = 1;

    return 0;
}

int thread_exit_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_EXIT");

    // TODO

    SHOULD_REDISPATCH = 0;

    return 0;
}

int mutex_create_kernel_syscall(t_Payload *syscall_arguments) {

    char *resource_name;
    text_deserialize(syscall_arguments, &resource_name);

    log_trace(MODULE_LOGGER, "MUTEX_CREATE %s", resource_name);

    // TODO

    SHOULD_REDISPATCH = 1;

    return 0;
}

int mutex_lock_kernel_syscall(t_Payload *syscall_arguments) {
    int status;

    char *resource_name;
    text_deserialize(syscall_arguments, &resource_name);

    log_trace(MODULE_LOGGER, "MUTEX_LOCK %s", resource_name);

    // TODO

    /*
    t_Resource *resource = resource_find(resource_name);
    if(resource == NULL) {
        log_trace(MODULE_LOGGER, "WAIT %s: recurso no encontrado", resource_name);
        free(resource_name);
        return -1;
    }

    if(status = pthread_mutex_lock(&(resource->mutex_instances))) {
        log_error_pthread_mutex_lock(status);
        // TODO
    }

    resource->instances--;

    if(resource->instances < 0) {
        if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        switch_process_state(SYSCALL_PCB, BLOCKED_MUTEX_STATE);

        if(status = pthread_mutex_lock(&(resource->shared_list_blocked.mutex))) {
            log_error_pthread_mutex_lock(status);
            // TODO
        }
            list_add(resource->shared_list_blocked.list, SYSCALL_PCB);
            log_info(MINIMAL_LOGGER, "PID: <%d> - Bloqueado por: <%s>", (int) SYSCALL_PCB->PID, resource_name);
            SYSCALL_PCB->shared_list_state = &(resource->shared_list_blocked);
        if(status = pthread_mutex_unlock(&(resource->shared_list_blocked.mutex))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        EXEC_PCB = 0;
    } else {
        if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }

        list_add(SYSCALL_PCB->assigned_resources, resource);

        EXEC_PCB = 1;
    }

    free(resource_name);
    */

    return 0;
}

int mutex_unlock_kernel_syscall(t_Payload *syscall_arguments) {

    char *resource_name;
    text_deserialize(syscall_arguments, &resource_name);

    log_trace(MODULE_LOGGER, "MUTEX_UNLOCK %s", resource_name);

    // TODO

    /*
    t_Resource *resource = resource_find(resource_name);
    if(resource == NULL) {
        log_trace(MODULE_LOGGER, "SIGNAL %s: recurso no encontrado", resource_name);
        free(resource_name);
        return -1;
    }

    free(resource_name);

    EXEC_PCB = 1;

    list_remove_by_condition_with_comparation(SYSCALL_PCB->assigned_resources, (bool (*)(void *, void *)) pointers_match, (void *) resource);

    if(status = pthread_mutex_lock(&(resource->mutex_instances))) {
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
      
        switch_process_state(pcb, READY_STATE);
    }
    else {
        if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
            log_error_pthread_mutex_unlock(status);
            // TODO
        }
    }
    */

    return 0;
}

int dump_memory_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "DUMP_MEMORY");

    // TODO

    SHOULD_REDISPATCH = 0;

    return 0;
}

int io_kernel_syscall(t_Payload *syscall_arguments) {

    switch_process_state(EXEC_TCB, BLOCKED_IO_READY_STATE); 
 
    pthread_mutex_lock(&(SHARED_LIST_BLOCKED_IO_READY.mutex)); 
        list_add(SHARED_LIST_BLOCKED_IO_READY.list, EXEC_TCB); 
        log_debug(MINIMAL_LOGGER, "## (%u:%u) - Bloqueado por: IO", EXEC_TCB->pcb->PID, EXEC_TCB->TID); 
        EXEC_TCB->shared_list_state = &(SHARED_LIST_BLOCKED_IO_READY); 
    pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_IO_READY.mutex)); 
 
    if(sem_post(&SEM_IO_DEVICE)) { 
        log_error_sem_post(); 
        return -1; 
    } 

    log_trace(MODULE_LOGGER, "IO");

    SHOULD_REDISPATCH = 0;

    return 0;
}