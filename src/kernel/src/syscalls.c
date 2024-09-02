#include "syscalls.h"

t_Syscall SYSCALLS[] = {
    [WAIT_CPU_OPCODE] = {.name = "WAIT" , .function = wait_kernel_syscall},
    [SIGNAL_CPU_OPCODE] = {.name = "SIGNAL" , .function = signal_kernel_syscall},
    [IO_GEN_SLEEP_CPU_OPCODE] = {.name = "IO_GEN_SLEEP" , .function = NULL},
    [IO_STDIN_READ_CPU_OPCODE] = {.name = "IO_STDIN_READ" , .function = NULL},
    [IO_STDOUT_WRITE_CPU_OPCODE] = {.name = "IO_STDOUT_WRITE" , .function = NULL},
    [IO_FS_CREATE_CPU_OPCODE] = {.name = "IO_FS_CREATE" , .function = NULL},
    [IO_FS_DELETE_CPU_OPCODE] = {.name = "IO_FS_DELETE" , .function = NULL},
    [IO_FS_TRUNCATE_CPU_OPCODE] = {.name = "IO_FS_TRUNCATE" , .function = NULL},
    [IO_FS_WRITE_CPU_OPCODE] = {.name = "IO_FS_WRITE" , .function = NULL},
    [IO_FS_READ_CPU_OPCODE] = {.name = "IO_FS_READ" , .function = NULL},
    [EXIT_CPU_OPCODE] = {.name = "EXIT" , .function = exit_kernel_syscall}
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

int wait_kernel_syscall(t_Payload *syscall_arguments) {

    char *resource_name;
    text_deserialize(syscall_arguments, &resource_name);

    log_trace(MODULE_LOGGER, "WAIT %s", resource_name);

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

int signal_kernel_syscall(t_Payload *syscall_arguments) {

    char *resource_name;
    text_deserialize(syscall_arguments, &resource_name);

    log_trace(MODULE_LOGGER, "SIGNAL %s", resource_name);

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

int exit_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "EXIT");

    EXEC_TCB = 0;

    return 0;
}