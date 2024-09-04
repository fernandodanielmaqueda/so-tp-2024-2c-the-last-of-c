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

t_TCB *SYSCALL_TCB;

int syscall_execute(t_Payload *syscall_instruction) {

    e_CPU_OpCode syscall_opcode;
    cpu_opcode_deserialize(syscall_instruction, &syscall_opcode);

    if(SYSCALLS[syscall_opcode].function == NULL) {
        payload_destroy(syscall_instruction);
        log_error(MODULE_LOGGER, "Funcion de syscall no encontrada");
        //SYSCALL_PCB->exit_reason = UNEXPECTED_ERROR_EXIT_REASON;
        return 1;
    }

    int exit_status = SYSCALLS[syscall_opcode].function(syscall_instruction);
    payload_destroy(syscall_instruction);
    return exit_status;
}

int process_create_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "PROCESS_CREATE");

    EXEC_TCB = 0;

    return 0;
}

int process_exit_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "PROCESS_EXIT");

    EXEC_TCB = 0;

    return 0;
}

int thread_create_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_CREATE");

    EXEC_TCB = 0;

    return 0;
}

int thread_join_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_JOIN");

    EXEC_TCB = 0;

    return 0;
}

int thread_cancel_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_CANCEL");

    EXEC_TCB = 0;

    return 0;
}

int thread_exit_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "THREAD_EXIT");

    EXEC_TCB = 0;

    return 0;
}

int mutex_create_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "MUTEX_CREATE");

    EXEC_TCB = 0;

    return 0;
}

int mutex_lock_kernel_syscall(t_Payload *syscall_arguments) {

    char *resource_name;
    text_deserialize(syscall_arguments, &resource_name);

    log_trace(MODULE_LOGGER, "MUTEX_LOCK %s", resource_name);

    /*
    t_Resource *resource = resource_find(resource_name);
    if(resource == NULL) {
        log_trace(MODULE_LOGGER, "WAIT %s: recurso no encontrado", resource_name);
        free(resource_name);
        return 1;
    }

    pthread_mutex_lock(&(resource->mutex_instances));

    resource->instances--;

    if(resource->instances < 0) {
        pthread_mutex_unlock(&(resource->mutex_instances));

        switch_process_state(SYSCALL_PCB, BLOCKED_STATE);

        pthread_mutex_lock(&(resource->shared_list_blocked.mutex));
            list_add(resource->shared_list_blocked.list, SYSCALL_PCB);
            log_info(MINIMAL_LOGGER, "PID: <%d> - Bloqueado por: <%s>", (int) SYSCALL_PCB->PID, resource_name);
            SYSCALL_PCB->shared_list_state = &(resource->shared_list_blocked);
        pthread_mutex_unlock(&(resource->shared_list_blocked.mutex));

        EXEC_PCB = 0;
    } else {
        pthread_mutex_unlock(&(resource->mutex_instances));

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

    /*
    t_Resource *resource = resource_find(resource_name);
    if(resource == NULL) {
        log_trace(MODULE_LOGGER, "SIGNAL %s: recurso no encontrado", resource_name);
        free(resource_name);
        return 1;
    }

    free(resource_name);

    EXEC_PCB = 1;

    list_remove_by_condition_with_comparation(SYSCALL_PCB->assigned_resources, (bool (*)(void *, void *)) pointers_match, (void *) resource);

    pthread_mutex_lock(&(resource->mutex_instances));

    resource->instances++;

    if(resource->instances <= 0) {
        pthread_mutex_unlock(&(resource->mutex_instances));

        pthread_mutex_unlock(&(resource->shared_list_blocked.mutex));

            if((resource->shared_list_blocked.list)->head == NULL) {
                pthread_mutex_unlock(&(resource->shared_list_blocked.mutex));
                return 0;
            }

            t_PCB *pcb = (t_PCB *) list_remove(resource->shared_list_blocked.list, 0);

        pthread_mutex_unlock(&(resource->shared_list_blocked.mutex));

        list_add(pcb->assigned_resources, resource);
      
        switch_process_state(pcb, READY_STATE);
    }
    else {
        pthread_mutex_unlock(&(resource->mutex_instances));
    }
    */

    return 0;
}

int dump_memory_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "MUTEX_CREATE");

    EXEC_TCB = 0;

    return 0;
}

int io_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "IO");

    EXEC_TCB = 0;

    return 0;
}