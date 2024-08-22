#include "syscalls.h"

t_Syscall SYSCALLS[] = {
    [WAIT_CPU_OPCODE] = {.name = "WAIT" , .function = wait_kernel_syscall},
    [SIGNAL_CPU_OPCODE] = {.name = "SIGNAL" , .function = signal_kernel_syscall},
    [IO_GEN_SLEEP_CPU_OPCODE] = {.name = "IO_GEN_SLEEP" , .function = io_gen_sleep_kernel_syscall},
    [IO_STDIN_READ_CPU_OPCODE] = {.name = "IO_STDIN_READ" , .function = io_stdin_read_kernel_syscall},
    [IO_STDOUT_WRITE_CPU_OPCODE] = {.name = "IO_STDOUT_WRITE" , .function = io_stdout_write_kernel_syscall},
    [IO_FS_CREATE_CPU_OPCODE] = {.name = "IO_FS_CREATE" , .function = io_fs_create_kernel_syscall},
    [IO_FS_DELETE_CPU_OPCODE] = {.name = "IO_FS_DELETE" , .function = io_fs_delete_kernel_syscall},
    [IO_FS_TRUNCATE_CPU_OPCODE] = {.name = "IO_FS_TRUNCATE" , .function = io_fs_truncate_kernel_syscall},
    [IO_FS_WRITE_CPU_OPCODE] = {.name = "IO_FS_WRITE" , .function = io_fs_write_kernel_syscall},
    [IO_FS_READ_CPU_OPCODE] = {.name = "IO_FS_READ" , .function = io_fs_read_kernel_syscall},
    [EXIT_CPU_OPCODE] = {.name = "EXIT" , .function = exit_kernel_syscall}
};

t_PCB *SYSCALL_PCB;

int syscall_execute(t_Payload *syscall_instruction) {

    e_CPU_OpCode syscall_opcode;
    cpu_opcode_deserialize(syscall_instruction, &syscall_opcode);

    if(SYSCALLS[syscall_opcode].function == NULL) {
        payload_destroy(syscall_instruction);
        log_error(MODULE_LOGGER, "Funcion de syscall no encontrada");
        SYSCALL_PCB->exit_reason = UNEXPECTED_ERROR_EXIT_REASON;
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

    t_Resource *resource = resource_find(resource_name);
    if(resource == NULL) {
        log_trace(MODULE_LOGGER, "WAIT %s: recurso no encontrado", resource_name);
        free(resource_name);
        SYSCALL_PCB->exit_reason = INVALID_RESOURCE_EXIT_REASON;
        return 1;
    }

    pthread_mutex_lock(&(resource->mutex_instances));

    resource->instances--;

    if(resource->instances < 0) {
        pthread_mutex_unlock(&(resource->mutex_instances));

        switch_process_state(SYSCALL_PCB, BLOCKED_STATE);

        pthread_mutex_lock(&(resource->shared_list_blocked.mutex));
            list_add(resource->shared_list_blocked.list, SYSCALL_PCB);
            log_debug(MINIMAL_LOGGER, "PID: <%d> - Bloqueado por: <%s>", (int) SYSCALL_PCB->exec_context.PID, resource_name);
            SYSCALL_PCB->shared_list_state = &(resource->shared_list_blocked);
        pthread_mutex_unlock(&(resource->shared_list_blocked.mutex));

        EXEC_PCB = 0;
    } else {
        pthread_mutex_unlock(&(resource->mutex_instances));

        list_add(SYSCALL_PCB->assigned_resources, resource);

        EXEC_PCB = 1;
    }

    free(resource_name);

    return 0;
}

int signal_kernel_syscall(t_Payload *syscall_arguments) {

    char *resource_name;
    text_deserialize(syscall_arguments, &resource_name);

    log_trace(MODULE_LOGGER, "SIGNAL %s", resource_name);

    t_Resource *resource = resource_find(resource_name);
    if(resource == NULL) {
        log_trace(MODULE_LOGGER, "SIGNAL %s: recurso no encontrado", resource_name);
        free(resource_name);
        SYSCALL_PCB->exit_reason = INVALID_RESOURCE_EXIT_REASON;
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

    return 0;
}

int io_gen_sleep_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_GEN_SLEEP_CPU_OPCODE, GENERIC_IO_TYPE);
}

int io_stdin_read_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_STDIN_READ_CPU_OPCODE, STDIN_IO_TYPE);
}

int io_stdout_write_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_STDOUT_WRITE_CPU_OPCODE, STDOUT_IO_TYPE);
}

int io_fs_create_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_FS_CREATE_CPU_OPCODE, DIALFS_IO_TYPE);
}

int io_fs_delete_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_FS_DELETE_CPU_OPCODE, DIALFS_IO_TYPE);
}

int io_fs_truncate_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_FS_TRUNCATE_CPU_OPCODE, DIALFS_IO_TYPE);
}

int io_fs_write_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_FS_WRITE_CPU_OPCODE, DIALFS_IO_TYPE);
}

int io_fs_read_kernel_syscall(t_Payload *syscall_arguments) {
    return process_io_syscall(syscall_arguments, IO_FS_READ_CPU_OPCODE, DIALFS_IO_TYPE);
}

int exit_kernel_syscall(t_Payload *syscall_arguments) {

    log_trace(MODULE_LOGGER, "EXIT");

    EXEC_PCB = 0;

    return 0;
}

int process_io_syscall(t_Payload *syscall_arguments, e_CPU_OpCode syscall_opcode, e_IO_Type required_io_type) {
    char *interface_name;
    text_deserialize(syscall_arguments, &interface_name);

    log_trace(MODULE_LOGGER, "%s %s", SYSCALLS[syscall_opcode].name, interface_name);

    EXEC_PCB = 0;

    wait_draining_requests(&INTERFACES_SYNC);

        t_Interface *interface = (t_Interface *) list_find_by_condition_with_comparation(LIST_INTERFACES, (bool (*)(void *, void *)) interface_name_matches, interface_name);
        if(interface == NULL) {
            log_warning(MODULE_LOGGER, "%s: la interfaz solicitada no existe y/o no esta conectada", interface_name);
            free(interface_name);
            SYSCALL_PCB->exit_reason = INVALID_INTERFACE_EXIT_REASON;
            return 1;
        }

        free(interface_name);

        if(interface->io_type != required_io_type) {
            log_warning(MODULE_LOGGER, "%s: la interfaz no admite la operacion solicitada", interface->name);
            SYSCALL_PCB->exit_reason = INVALID_INTERFACE_EXIT_REASON;
            return 1;
        }

        cpu_opcode_serialize(&(SYSCALL_PCB->io_operation), syscall_opcode);
        payload_add(&(SYSCALL_PCB->io_operation), syscall_arguments->stream, (size_t) syscall_arguments->size);

        switch_process_state(SYSCALL_PCB, BLOCKED_STATE);

        pthread_mutex_lock(&(interface->shared_list_blocked_ready.mutex));

            list_add(interface->shared_list_blocked_ready.list, SYSCALL_PCB);
            log_debug(MINIMAL_LOGGER, "PID: <%d> - Bloqueado por: <%s>", (int) SYSCALL_PCB->exec_context.PID, interface->name);
            SYSCALL_PCB->shared_list_state = &(interface->shared_list_blocked_ready);

            sem_post(&(interface->sem_scheduler));
        
        pthread_mutex_unlock(&(interface->shared_list_blocked_ready.mutex));
        
    signal_draining_requests(&INTERFACES_SYNC);

    return 0;
}