/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "opcodes.h"

t_CPU_Operation CPU_OPERATIONS[] = {
    [SET_CPU_OPCODE] = {.function = set_cpu_operation},
    [READ_MEM_CPU_OPCODE] = {.function = read_mem_cpu_operation}, 
    [WRITE_MEM_CPU_OPCODE] = {.function = write_mem_cpu_operation},
    [SUM_CPU_OPCODE] = {.function = sum_cpu_operation},
    [SUB_CPU_OPCODE] = {.function = sub_cpu_operation},
    [JNZ_CPU_OPCODE] = {.function = jnz_cpu_operation},
    [LOG_CPU_OPCODE] = {.function = log_cpu_operation},
    [PROCESS_CREATE_CPU_OPCODE] = {.function = process_create_cpu_operation},
    [PROCESS_EXIT_CPU_OPCODE] = {.function = process_exit_cpu_operation},
    [THREAD_CREATE_CPU_OPCODE] = {.function = thread_create_cpu_operation},
    [THREAD_JOIN_CPU_OPCODE] = {.function = thread_join_cpu_operation},
    [THREAD_CANCEL_CPU_OPCODE] = {.function = thread_cancel_cpu_operation},
    [THREAD_EXIT_CPU_OPCODE] = {.function = thread_exit_cpu_operation},
    [MUTEX_CREATE_CPU_OPCODE] = {.function = mutex_create_cpu_operation},
    [MUTEX_LOCK_CPU_OPCODE] = {.function = mutex_lock_cpu_operation},
    [MUTEX_UNLOCK_CPU_OPCODE] = {.function = mutex_unlock_cpu_operation},
    [DUMP_MEMORY_CPU_OPCODE] = {.function = dump_memory_cpu_operation},
    [IO_CPU_OPCODE] = {.function = io_cpu_operation}
};

int decode_instruction(char *name, e_CPU_OpCode *destination) {
    if(name == NULL || destination == NULL)
        return -1;

    for (register e_CPU_OpCode cpu_opcode = 0; CPU_OPCODE_NAMES[cpu_opcode] != NULL; cpu_opcode++)
        if(strcmp(CPU_OPCODE_NAMES[cpu_opcode], name) == 0) {
            *destination = cpu_opcode;
            return 0;
        }

    return -1;
}

int set_cpu_operation(int argc, char **argv) {
    if(argc != 3) {
        log_warning_r(&MODULE_LOGGER, "Uso: SET <REGISTRO> <VALOR>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s", TID, argv[0], argv[1], argv[2]);

    e_CPU_Register destination_register;
    if(decode_register(argv[1], &destination_register)) {
        log_warning_r(&MODULE_LOGGER, "<REGISTRO> %s no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    uint32_t value;
    if(str_to_uint32(argv[2], &value)) {
        log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    if(set_register_value(&EXEC_CONTEXT, destination_register, value)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al asignar valor a %s", argv[1], argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    EXEC_CONTEXT.cpu_registers.PC++;

    SYSCALL_CALLED = 0;
    return 0;
}

int read_mem_cpu_operation(int argc, char **argv) {
    if(argc != 3) {
        log_warning_r(&MODULE_LOGGER, "Uso: READ_MEM <REGISTRO DATOS> <REGISTRO DIRECCION>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s", TID, argv[0], argv[1], argv[2]);

    e_CPU_Register register_data;
    if(decode_register(argv[1], &register_data)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    e_CPU_Register register_address;
    if(decode_register(argv[2], &register_address)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    uint32_t logical_address;
    if(get_register_value(EXEC_CONTEXT, register_address, &logical_address)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[register_address].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    size_t bytes = get_register_size(register_data);

    size_t physical_address;
    if(mmu((size_t) logical_address, bytes, &physical_address)) {
        switch(errno) {
            case EFAULT:
                EVICTION_REASON = SEGMENTATION_FAULT_EVICTION_REASON;
                return -1;
            default:
                EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                return -1;
        }
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Acción: LEER - Dirección Física: %zu", TID, physical_address);

    void *destination = get_register_pointer(&EXEC_CONTEXT, register_data);

    int result;
    request_memory_read(physical_address, destination, bytes, &result);
    if(result) {
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    EXEC_CONTEXT.cpu_registers.PC++;

    SYSCALL_CALLED = 0;
    return 0;
}

int write_mem_cpu_operation(int argc, char **argv) {
    if(argc != 3) {
        log_warning_r(&MODULE_LOGGER, "Uso: WRITE_MEM <REGISTRO DIRECCION> <REGISTRO DATOS>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s", TID, argv[0], argv[1], argv[2]);

    e_CPU_Register register_address;
    if(decode_register(argv[1], &register_address)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    e_CPU_Register register_data;
    if(decode_register(argv[2], &register_data)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    void *source = get_register_pointer(&EXEC_CONTEXT, register_data);
    size_t bytes = get_register_size(register_data);

    uint32_t logical_address;
    if(get_register_value(EXEC_CONTEXT, register_address, &logical_address)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[register_address].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    size_t physical_address;
    if(mmu((size_t) logical_address, bytes, &physical_address)) {
        switch(errno) {
            case EFAULT:
                EVICTION_REASON = SEGMENTATION_FAULT_EVICTION_REASON;
                return -1;
            default:
                EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
                return -1;
        }
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Acción: ESCRIBIR - Dirección Física: %zu", TID, physical_address);

    int result;
    request_memory_write(physical_address, source, bytes, &result);
    if(result) {
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    EXEC_CONTEXT.cpu_registers.PC++;

    SYSCALL_CALLED = 0;
    return 0;
}

int sum_cpu_operation(int argc, char **argv) {
    if(argc != 3) {
        log_warning_r(&MODULE_LOGGER, "Uso: SUM <REGISTRO DESTINO> <REGISTRO ORIGEN>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s", TID, argv[0], argv[1], argv[2]);

    e_CPU_Register register_destination;
    if(decode_register(argv[1], &register_destination)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    e_CPU_Register register_origin;
    if(decode_register(argv[2], &register_origin)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    uint32_t value_register_destination;
    uint32_t value_register_origin;
    if(get_register_value(EXEC_CONTEXT, register_destination, &value_register_destination)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[register_destination].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }
    if(get_register_value(EXEC_CONTEXT, register_origin, &value_register_origin)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[register_origin].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    if(set_register_value(&EXEC_CONTEXT, register_destination, (value_register_destination + value_register_origin))) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al asignar valor", CPU_REGISTERS[register_destination].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    EXEC_CONTEXT.cpu_registers.PC++;

    SYSCALL_CALLED = 0;
    return 0;
}

int sub_cpu_operation(int argc, char **argv) {
    if(argc != 3) {
        log_warning_r(&MODULE_LOGGER, "Uso: SUB <REGISTRO DESTINO> <REGISTRO ORIGEN>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s", TID, argv[0], argv[1], argv[2]);

    e_CPU_Register register_destination;
    if(decode_register(argv[1], &register_destination)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    e_CPU_Register register_origin;
    if(decode_register(argv[2], &register_origin)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    uint32_t value_register_destination;
    uint32_t value_register_origin;
    if(get_register_value(EXEC_CONTEXT, register_destination, &value_register_destination)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[register_destination].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }
    if(get_register_value(EXEC_CONTEXT, register_origin, &value_register_origin)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[register_origin].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    if(set_register_value(&EXEC_CONTEXT, register_destination, (value_register_destination - value_register_origin))) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al asignar valor", CPU_REGISTERS[register_destination].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    EXEC_CONTEXT.cpu_registers.PC++;

    SYSCALL_CALLED = 0;
    return 0;
}

int jnz_cpu_operation(int argc, char **argv) {
    if(argc != 3) {
        log_warning_r(&MODULE_LOGGER, "Uso: JNZ <REGISTRO> <INSTRUCCION>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s", TID, argv[0], argv[1], argv[2]);

    e_CPU_Register cpu_register;
    if(decode_register(argv[1], &cpu_register)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    t_PC instruction;
    if(str_to_pc(argv[2], &instruction)) {
        log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    uint32_t value_cpu_register;
    if(get_register_value(EXEC_CONTEXT, cpu_register, &value_cpu_register)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[cpu_register].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    if(value_cpu_register)
        EXEC_CONTEXT.cpu_registers.PC = instruction;
    else
        EXEC_CONTEXT.cpu_registers.PC++;
    
    SYSCALL_CALLED = 0;
    return 0;
}

int log_cpu_operation(int argc, char **argv) {
    if(argc != 2) {
        log_warning_r(&MODULE_LOGGER, "Uso: LOG <REGISTRO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s", TID, argv[0], argv[1]);

    e_CPU_Register cpu_register;
    if(decode_register(argv[1], &cpu_register)) {
        log_warning_r(&MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    uint32_t value_cpu_register;
    if(get_register_value(EXEC_CONTEXT, cpu_register, &value_cpu_register)) {
        log_warning_r(&MODULE_LOGGER, "%s: Error al obtener valor", CPU_REGISTERS[cpu_register].name);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MODULE_LOGGER, "(%u:%u) LOG %s: %u", PID, TID, argv[1], value_cpu_register);

    EXEC_CONTEXT.cpu_registers.PC++;

    SYSCALL_CALLED = 0;
    return 0;
}

int process_create_cpu_operation(int argc, char **argv) {
    if(argc != 4) {
        log_warning_r(&MODULE_LOGGER, "Uso: PROCESS_CREATE <ARCHIVO DE INSTRUCCIONES> <TAMANIO> <PRIORIDAD DEL TID 0>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s %s", TID, argv[0], argv[1], argv[2], argv[3]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, PROCESS_CREATE_CPU_OPCODE)) {
        exit_sigint();
    }
    if(text_serialize(&SYSCALL_INSTRUCTION, argv[1])) {
        exit_sigint();
    }
        size_t size;
        if(str_to_size(argv[2], &size)) {
            log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[2]);
            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
            return -1;
        }
    if(size_serialize(&SYSCALL_INSTRUCTION, size)) {
        exit_sigint();
    }
        t_Priority priority;
        if(str_to_priority(argv[3], &priority)) {
            log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[3]);
            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
            return -1;
        }
    if(payload_add(&SYSCALL_INSTRUCTION, &priority, sizeof(t_Priority))) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int process_exit_cpu_operation(int argc, char **argv) {
    if(argc != 1) {
        log_warning_r(&MODULE_LOGGER, "Uso: PROCESS_EXIT");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s", TID, argv[0]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, PROCESS_EXIT_CPU_OPCODE)) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int thread_create_cpu_operation(int argc, char **argv) {
    if(argc != 3) {
        log_warning_r(&MODULE_LOGGER, "Uso: THREAD_CREATE <ARCHIVO DE INSTRUCCIONES> <PRIORIDAD>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s %s", TID, argv[0], argv[1], argv[2]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, THREAD_CREATE_CPU_OPCODE)) {
        exit_sigint();
    }
    if(text_serialize(&SYSCALL_INSTRUCTION, argv[1])) {
        exit_sigint();
    }
        t_Priority priority;
        if(str_to_priority(argv[2], &priority)) {
            log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[2]);
            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
            return -1;
        }
    if(payload_add(&SYSCALL_INSTRUCTION, &priority, sizeof(priority))) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int thread_join_cpu_operation(int argc, char **argv) {
    if(argc != 2) {
        log_warning_r(&MODULE_LOGGER, "Uso: THREAD_JOIN <TID>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s", TID, argv[0], argv[1]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, THREAD_JOIN_CPU_OPCODE)) {
        exit_sigint();
    }
        t_TID tid;
        if(str_to_tid(argv[1], &tid)) {
            log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[1]);
            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
            return -1;
        }
    if(payload_add(&SYSCALL_INSTRUCTION, &tid, sizeof(tid))) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int thread_cancel_cpu_operation(int argc, char **argv) {
    if(argc != 2) {
        log_warning_r(&MODULE_LOGGER, "Uso: THREAD_CANCEL <TID>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s", TID, argv[0], argv[1]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, THREAD_CANCEL_CPU_OPCODE)) {
        exit_sigint();
    }
        t_TID tid;
        if(str_to_tid(argv[1], &tid)) {
            log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[1]);
            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
            return -1;
        }
    if(payload_add(&SYSCALL_INSTRUCTION, &tid, sizeof(tid))) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int thread_exit_cpu_operation(int argc, char **argv) {
    if(argc != 1) {
        log_warning_r(&MODULE_LOGGER, "Uso: THREAD_EXIT");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s", TID, argv[0]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, THREAD_EXIT_CPU_OPCODE)) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int mutex_create_cpu_operation(int argc, char **argv) {
    if(argc != 2) {
        log_warning_r(&MODULE_LOGGER, "Uso: MUTEX_CREATE <RECURSO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s", TID, argv[0], argv[1]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, MUTEX_CREATE_CPU_OPCODE)) {
        exit_sigint();
    }
    if(text_serialize(&SYSCALL_INSTRUCTION, argv[1])) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int mutex_lock_cpu_operation(int argc, char **argv) {
    if(argc != 2) {
        log_warning_r(&MODULE_LOGGER, "Uso: MUTEX_LOCK <RECURSO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s", TID, argv[0], argv[1]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, MUTEX_LOCK_CPU_OPCODE)) {
        exit_sigint();
    }
    if(text_serialize(&SYSCALL_INSTRUCTION, argv[1])) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int mutex_unlock_cpu_operation(int argc, char **argv) {
    if(argc != 2) {
        log_warning_r(&MODULE_LOGGER, "Uso: MUTEX_UNLOCK <RECURSO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s", TID, argv[0]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, MUTEX_UNLOCK_CPU_OPCODE)) {
        exit_sigint();
    }
    if(text_serialize(&SYSCALL_INSTRUCTION, argv[1])) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int dump_memory_cpu_operation(int argc, char **argv) {
    if(argc != 1) {
        log_warning_r(&MODULE_LOGGER, "Uso: DUMP_MEMORY");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s", TID, argv[0]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, DUMP_MEMORY_CPU_OPCODE)) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}

int io_cpu_operation(int argc, char **argv) {
    if(argc != 2) {
        log_warning_r(&MODULE_LOGGER, "Uso: IO <TIEMPO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return -1;
    }

    log_info_r(&MINIMAL_LOGGER, "## TID: %u - Ejecutando: %s %s", TID, argv[0], argv[1]);

    EXEC_CONTEXT.cpu_registers.PC++;

    if(cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_CPU_OPCODE)) {
        exit_sigint();
    }
        t_Time time;
        if(str_to_time(argv[1], &time)) {
            log_warning_r(&MODULE_LOGGER, "%s: No es un valor valido", argv[1]);
            EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
            return -1;
        }
    if(payload_add(&SYSCALL_INSTRUCTION, &time, sizeof(time))) {
        exit_sigint();
    }

    SYSCALL_CALLED = 1;
    return 0;
}