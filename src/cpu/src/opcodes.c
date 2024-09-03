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
    [EXIT_CPU_OPCODE] = {.function = exit_cpu_operation}
};

int decode_instruction(char *name, e_CPU_OpCode *destination)
{
    
    if(name == NULL || destination == NULL)
        return 1;

    for (register e_CPU_OpCode cpu_opcode = 0; CPU_OPCODE_NAMES[cpu_opcode] != NULL; cpu_opcode++)
        if (strcmp(CPU_OPCODE_NAMES[cpu_opcode], name) == 0) {
            *destination = cpu_opcode;
            return 0;
        }

    return 1;
}

int set_cpu_operation(int argc, char **argv)
{

    if (argc != 3) {
        log_error(MODULE_LOGGER, "Uso: SET <REGISTRO> <VALOR>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "SET %s %s", argv[1], argv[2]);

    e_CPU_Register destination_register;
    if(decode_register(argv[1], &destination_register)) {
        log_error(MODULE_LOGGER, "<REGISTRO> %s no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t value;
    if(str_to_uint32(argv[2], &value)) {
        log_error(MODULE_LOGGER, "%s: No es un valor valido", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    set_register_value(&EXEC_CONTEXT, destination_register, value);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int read_mem_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: MOV_IN <REGISTRO DATOS> <REGISTRO DIRECCION>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "Read mem %s %s", argv[1], argv[2]);

    e_CPU_Register register_data;
    if (decode_register(argv[1], &register_data)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_address;
    if(decode_register(argv[2], &register_address)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t logical_address;
    get_register_value(EXEC_CONTEXT, register_address, &logical_address);

    size_t bytes = get_register_size(register_data);

    t_list *list_physical_addresses = mmu(EXEC_CONTEXT.PID, (size_t) logical_address, bytes);
    if(list_physical_addresses == NULL || list_size(list_physical_addresses) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        // free_list_physical_addresses(list_physical_addresses);
        return 1;
    }

    void *destination = get_register_pointer(&EXEC_CONTEXT, register_data);
    void *source = malloc((size_t) bytes);
    if(source == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zd bytes", (size_t) bytes);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        // free_list_physical_addresses(list_physical_addresses);
        return 1;
    }

    //attend_read(EXEC_CONTEXT.PID, list_physical_addresses, &source, bytes);
    memcpy(destination, source, bytes);
    free(source);

    // free_list_physical_addresses(list_physical_addresses);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int write_mem_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: WRITE_MEM <REGISTRO DIRECCION> <REGISTRO DATOS>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "WRITE_MEM %s %s", argv[1], argv[2]);

    e_CPU_Register register_address;
    if(decode_register(argv[1], &register_address)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_data;
    if(decode_register(argv[2], &register_data)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    void *source = get_register_pointer(&EXEC_CONTEXT, register_data);
    size_t bytes = get_register_size(register_data);

    uint32_t logical_address;
    get_register_value(EXEC_CONTEXT, register_address, &logical_address);

    char text[bytes+1];
    memcpy(text, source, bytes);
    text[bytes] = '\0';

    t_list *list_physical_addresses = mmu(EXEC_CONTEXT.PID, (size_t) logical_address, bytes);
    if(list_physical_addresses == NULL || list_size(list_physical_addresses) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    //attend_write(EXEC_CONTEXT.PID, list_physical_addresses, text, bytes);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int sum_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: SUM <REGISTRO DESTINO> <REGISTRO ORIGEN>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "SUM %s %s", argv[1], argv[2]);

    e_CPU_Register register_destination;
    if (decode_register(argv[1], &register_destination)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_origin;
    if(decode_register(argv[2], &register_origin)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t value_register_destination;
    uint32_t value_register_origin;
    get_register_value(EXEC_CONTEXT, register_destination, &value_register_destination);
    get_register_value(EXEC_CONTEXT, register_origin, &value_register_origin);

    set_register_value(&EXEC_CONTEXT, register_destination, (value_register_destination + value_register_origin));

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int sub_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: SUB <REGISTRO DESTINO> <REGISTRO ORIGEN>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "SUB %s %s", argv[1], argv[2]);

    e_CPU_Register register_destination;
    if (decode_register(argv[1], &register_destination)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_origin;
    if(decode_register(argv[2], &register_origin)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t value_register_destination;
    uint32_t value_register_origin;
    get_register_value(EXEC_CONTEXT, register_destination, &value_register_destination);
    get_register_value(EXEC_CONTEXT, register_origin, &value_register_origin);

    set_register_value(&EXEC_CONTEXT, register_destination, (value_register_destination - value_register_origin));

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int jnz_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: JNZ <REGISTRO> <INSTRUCCION>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "JNZ %s %s", argv[1], argv[2]);

    e_CPU_Register cpu_register;
    if(decode_register(argv[1], &cpu_register)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    t_PC instruction;
    if(str_to_pc(argv[2], &instruction)) {
        log_error(MODULE_LOGGER, "%s: No es un valor valido", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t value_cpu_register;
    get_register_value(EXEC_CONTEXT, cpu_register, &value_cpu_register);

    if(value_cpu_register)
        EXEC_CONTEXT.PC = instruction;
    else
        EXEC_CONTEXT.PC++;
    
    SYSCALL_CALLED = 0;

    return 0;
}

// LOG (Registro): Escribe en el archivo de log el valor del registro. -- VERIFICAR
int log_cpu_operation(int argc, char **argv)
{

    if (argc != 2)
    {
        log_error(MODULE_LOGGER, "Uso: LOG <REGISTRO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "LOG %s", argv[1]);

    e_CPU_Register cpu_register;
    if(decode_register(argv[1], &cpu_register)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t value_cpu_register;
    get_register_value(EXEC_CONTEXT, cpu_register, &value_cpu_register);

    log_info(MODULE_LOGGER, "Registro %s: %d", argv[1], value_cpu_register);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}
//////////////////////////////////////////////////////////////////////////////////

int exit_cpu_operation(int argc, char **argv)
{
    log_trace(MODULE_LOGGER, "EXIT");

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int str_to_uint32(char *string, uint32_t *destination)
{
    char *end;

    *destination = (uint32_t) strtoul(string, &end, 10);

    if(!*string || *end)
        return 1;
        
    return 0;
}

int str_to_size(char *string, size_t *destination)
{
    char *end;

    *destination = (size_t) strtoul(string, &end, 10);

    if(!*string || *end)
        return 1;
        
    return 0;
}


int str_to_pc(char *string, t_PC *destination)
{
    char *end;

    *destination = (t_PC) strtoul(string, &end, 10);

    if(!*string || *end)
        return 1;
        
    return 0;
}