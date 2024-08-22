/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "opcodes.h"

t_CPU_Operation CPU_OPERATIONS[] = {
    [SET_CPU_OPCODE] = {.function = set_cpu_operation},
    [MOV_IN_CPU_OPCODE] = {.function = mov_in_cpu_operation},
    [MOV_OUT_CPU_OPCODE] = {.function = mov_out_cpu_operation},
    [SUM_CPU_OPCODE] = {.function = sum_cpu_operation},
    [SUB_CPU_OPCODE] = {.function = sub_cpu_operation},
    [JNZ_CPU_OPCODE] = {.function = jnz_cpu_operation},
    [RESIZE_CPU_OPCODE] = {.function = resize_cpu_operation},
    [COPY_STRING_CPU_OPCODE] = {.function = copy_string_cpu_operation},
    [WAIT_CPU_OPCODE] = {.function = wait_cpu_operation},
    [SIGNAL_CPU_OPCODE] = {.function = signal_cpu_operation},
    [IO_GEN_SLEEP_CPU_OPCODE] = {.function = io_gen_sleep_cpu_operation},
    [IO_STDIN_READ_CPU_OPCODE] = {.function = io_stdin_read_cpu_operation},
    [IO_STDOUT_WRITE_CPU_OPCODE] = {.function = io_stdout_write_cpu_operation},
    [IO_FS_CREATE_CPU_OPCODE] = {.function = io_fs_create_cpu_operation},
    [IO_FS_DELETE_CPU_OPCODE] = {.function = io_fs_delete_cpu_operation},
    [IO_FS_TRUNCATE_CPU_OPCODE] = {.function = io_fs_truncate_cpu_operation},
    [IO_FS_WRITE_CPU_OPCODE] = {.function = io_fs_write_cpu_operation},
    [IO_FS_READ_CPU_OPCODE] = {.function = io_fs_read_cpu_operation},
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

int mov_in_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: MOV_IN <REGISTRO DATOS> <REGISTRO DIRECCION>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "MOV_IN %s %s", argv[1], argv[2]);

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

    attend_read(EXEC_CONTEXT.PID, list_physical_addresses, &source, bytes);
    memcpy(destination, source, bytes);
    free(source);

    // free_list_physical_addresses(list_physical_addresses);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int mov_out_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: MOV_OUT <REGISTRO DIRECCION> <REGISTRO DATOS>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "MOV_OUT %s %s", argv[1], argv[2]);

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

    attend_write(EXEC_CONTEXT.PID, list_physical_addresses, text, bytes);

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

int resize_cpu_operation(int argc, char **argv)
{

    if (argc != 2)
    {
        log_error(MODULE_LOGGER, "Uso: RESIZE <TAMANIO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    size_t size;
    if(str_to_size(argv[1], &size)) {
        log_error(MODULE_LOGGER, "%s: No es un tamaño valido", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "RESIZE %s", argv[1]);

    t_Package *package = package_create_with_header(RESIZE_REQUEST);
    payload_add(&(package->payload), &(EXEC_CONTEXT.PID), sizeof(EXEC_CONTEXT.PID));
	payload_add(&(package->payload), &size, sizeof(size));
	package_send(package, CONNECTION_MEMORY.fd_connection);
	package_destroy(package);

    t_Return_Value return_value;
    if(receive_return_value_with_expected_header(RESIZE_REQUEST, &return_value, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
    if(return_value) {
        EVICTION_REASON = OUT_OF_MEMORY_EVICTION_REASON;
        return 1;
    }

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int copy_string_cpu_operation(int argc, char **argv)
{

    if (argc != 2)
    {
        log_error(MODULE_LOGGER, "Uso: COPY_STRING <TAMANIO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    size_t size;
    if(str_to_size(argv[1], &size)) {
        log_error(MODULE_LOGGER, "%s: No es un tamaño valido", argv[1]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "COPY_STRING %s", argv[1]);

    //REGISTRO ORIGEN
    uint32_t logical_address_si;
    get_register_value(EXEC_CONTEXT, SI_REGISTER, &logical_address_si);
    t_list *list_physical_addresses_si = mmu(EXEC_CONTEXT.PID, (size_t) logical_address_si, size);
    if(list_physical_addresses_si == NULL || list_size(list_physical_addresses_si) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas del registro origen.");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    //REGISTRO DESTINO
    uint32_t logical_address_di;
    get_register_value(EXEC_CONTEXT, DI_REGISTER, &logical_address_di);
    t_list *list_physical_addresses_di = mmu(EXEC_CONTEXT.PID, (size_t) logical_address_di, size);
    if(list_physical_addresses_di == NULL || list_size(list_physical_addresses_di) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas del registro destino.");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }
 /*    
    //Declaro variable donde se guardara
    void *source = malloc((size_t) size);
    if(source == NULL) {
        log_error(MODULE_LOGGER, "malloc: No se pudieron reservar %zd bytes", (size_t) size);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        // free_list_physical_addresses(list_physical_addresses);
        return 1;
    }

	log_debug(MINIMAL_LOGGER, "ANTES DEL READ");
    attend_read(EXEC_CONTEXT.PID, list_physical_addresses_si, &source, size); 
	log_debug(MINIMAL_LOGGER, "DESPUES DEL READ");

	log_debug(MINIMAL_LOGGER, "ANTES DEL WRITE");
    attend_write(EXEC_CONTEXT.PID, list_physical_addresses_di, source, size);
	log_debug(MINIMAL_LOGGER, "DESPUES DEL WRITE");

    free(source);
*/
  
    attend_copy(EXEC_CONTEXT.PID, list_physical_addresses_si, list_physical_addresses_di, size);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 0;

    return 0;
}

int wait_cpu_operation(int argc, char **argv)
{

    if (argc != 2)
    {
        log_error(MODULE_LOGGER, "Uso: WAIT <RECURSO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "WAIT %s", argv[1]);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, WAIT_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);

    return 0;
}

int signal_cpu_operation(int argc, char **argv)
{

    if (argc != 2)
    {
        log_error(MODULE_LOGGER, "Uso: SIGNAL <RECURSO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "SIGNAL %s", argv[1]);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, SIGNAL_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);

    return 0;
}

int io_gen_sleep_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: IO_GEN_SLEEP <INTERFAZ> <UNIDADES DE TRABAJO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_GEN_SLEEP %s %s", argv[1], argv[2]);

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_GEN_SLEEP_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    text_serialize(&SYSCALL_INSTRUCTION, argv[2]);

    return 0;
}

int io_stdin_read_cpu_operation(int argc, char **argv)
{

    if (argc != 4)
    {
        log_error(MODULE_LOGGER, "Uso: IO_STDIN_READ <INTERFAZ> <REGISTRO DIRECCION> <REGISTRO TAMANIO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_STDIN_READ %s %s %s", argv[1], argv[2], argv[3]);

    e_CPU_Register register_address;
    if(decode_register(argv[2], &register_address)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_size;
    if (decode_register(argv[3], &register_size)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[3]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t size;
    get_register_value(EXEC_CONTEXT, register_size, &size);

    uint32_t logical_address;
    get_register_value(EXEC_CONTEXT, register_address, &logical_address);

    t_list *list_physical_addresses = mmu(EXEC_CONTEXT.PID, (size_t) logical_address, (size_t) size);
    if(list_physical_addresses == NULL || list_size(list_physical_addresses) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_STDIN_READ_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    size_serialize(&SYSCALL_INSTRUCTION, (size_t) size);
    list_serialize(&SYSCALL_INSTRUCTION, *list_physical_addresses, size_serialize_element);

    return 0;
}

int io_stdout_write_cpu_operation(int argc, char **argv)
{

    if (argc != 4)
    {
        log_error(MODULE_LOGGER, "Uso: IO_STDOUT_WRITE <INTERFAZ> <REGISTRO DIRECCION> <REGISTRO TAMANIO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_STDOUT_WRITE %s %s %s", argv[1], argv[2], argv[3]);

    e_CPU_Register register_address;
    if(decode_register(argv[2], &register_address)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_size;
    if (decode_register(argv[3], &register_size)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[3]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    uint32_t size;
    get_register_value(EXEC_CONTEXT, register_size, &size);

    uint32_t logical_address;
    get_register_value(EXEC_CONTEXT, register_address, &logical_address);

    t_list *list_physical_addresses = mmu(EXEC_CONTEXT.PID, (size_t) logical_address, (size_t) size);
    if(list_physical_addresses == NULL || list_size(list_physical_addresses) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    EXEC_CONTEXT.PC++;

    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_STDOUT_WRITE_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    size_serialize(&SYSCALL_INSTRUCTION, (size_t) size);
    list_serialize(&SYSCALL_INSTRUCTION, *list_physical_addresses, size_serialize_element);

    return 0;
}

int io_fs_create_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: IO_FS_CREATE <INTERFAZ> <NOMBRE ARCHIVO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_FS_CREATE %s %s", argv[1], argv[2]);

    EXEC_CONTEXT.PC++;
    
    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_FS_CREATE_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    text_serialize(&SYSCALL_INSTRUCTION, argv[2]);

    return 0;
}

int io_fs_delete_cpu_operation(int argc, char **argv)
{

    if (argc != 3)
    {
        log_error(MODULE_LOGGER, "Uso: IO_FS_DELETE <INTERFAZ> <NOMBRE ARCHIVO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_FS_DELETE %s %s", argv[1], argv[2]);

    EXEC_CONTEXT.PC++;
    
    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_FS_DELETE_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    text_serialize(&SYSCALL_INSTRUCTION, argv[2]);

    return 0;
}

int io_fs_truncate_cpu_operation(int argc, char **argv)
{

    if (argc != 4)
    {
        log_error(MODULE_LOGGER, "Uso: IO_FS_TRUNCATE <INTERFAZ> <NOMBRE ARCHIVO> <REGISTRO TAMANIO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }
    e_CPU_Register register_size;
    if(decode_register(argv[3], &register_size)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[2]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_FS_TRUNCATE %s %s %s", argv[1], argv[2], argv[3]);

    uint32_t bytes = 0;
    get_register_value(EXEC_CONTEXT, register_size, &bytes);

    EXEC_CONTEXT.PC++;
    
    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_FS_TRUNCATE_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    text_serialize(&SYSCALL_INSTRUCTION, argv[2]);
    size_serialize(&SYSCALL_INSTRUCTION, (size_t) bytes);

    return 0;
}

int io_fs_write_cpu_operation(int argc, char **argv)
{
    if (argc != 6)
    {
        log_error(MODULE_LOGGER, "Uso: IO_FS_WRITE <INTERFAZ> <NOMBRE ARCHIVO> <REGISTRO DIRECCION> <REGISTRO TAMANIO> <REGISTRO PUNTERO ARCHIVO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_address;
    if(decode_register(argv[3], &register_address)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[3]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_size;
    if (decode_register(argv[4], &register_size)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[4]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_pointer;
    if (decode_register(argv[5], &register_pointer)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[5]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_FS_WRITE %s %s %s %s %s", argv[1], argv[2], argv[3], argv[4], argv[5]);

    uint32_t logical_address;
    uint32_t bytes;
    uint32_t pointer;

    get_register_value(EXEC_CONTEXT, register_address, &logical_address);
    get_register_value(EXEC_CONTEXT, register_pointer, &pointer);
    get_register_value(EXEC_CONTEXT, register_size, &bytes);

    t_list *list_physical_addresses = mmu(EXEC_CONTEXT.PID, (size_t) logical_address, (size_t) bytes);
    if(list_physical_addresses == NULL || list_size(list_physical_addresses) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    EXEC_CONTEXT.PC++;
    
    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_FS_WRITE_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    text_serialize(&SYSCALL_INSTRUCTION, argv[2]);
    size_serialize(&SYSCALL_INSTRUCTION, pointer);
    size_serialize(&SYSCALL_INSTRUCTION, bytes);
    list_serialize(&SYSCALL_INSTRUCTION, *list_physical_addresses, size_serialize_element);

    return 0;
}

int io_fs_read_cpu_operation(int argc, char **argv)
{
    if (argc != 6)
    {
        log_error(MODULE_LOGGER, "Uso: IO_FS_READ <INTERFAZ> <NOMBRE ARCHIVO> <REGISTRO DIRECCION> <REGISTRO TAMANIO> <REGISTRO PUNTERO ARCHIVO>");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_address;
    if(decode_register(argv[3], &register_address)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[3]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_size;
    if (decode_register(argv[4], &register_size)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[4]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    e_CPU_Register register_pointer;
    if (decode_register(argv[5], &register_pointer)) {
        log_error(MODULE_LOGGER, "%s: Registro no encontrado", argv[5]);
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "IO_FS_READ %s %s %s %s %s", argv[1], argv[2], argv[3], argv[4], argv[5]);

    uint32_t logical_address;
    uint32_t bytes;
    uint32_t pointer;

    get_register_value(EXEC_CONTEXT, register_address, &logical_address);
    get_register_value(EXEC_CONTEXT, register_size, &bytes);
    get_register_value(EXEC_CONTEXT, register_pointer, &pointer);

    t_list *list_physical_addresses = mmu(EXEC_CONTEXT.PID, (size_t) logical_address, (size_t) bytes);
    if(list_physical_addresses == NULL || list_size(list_physical_addresses) == 0) {
        log_error(MODULE_LOGGER, "mmu: No se pudo obtener la lista de direcciones fisicas");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    EXEC_CONTEXT.PC++;
    
    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, IO_FS_READ_CPU_OPCODE);
    text_serialize(&SYSCALL_INSTRUCTION, argv[1]);
    text_serialize(&SYSCALL_INSTRUCTION, argv[2]);
    size_serialize(&SYSCALL_INSTRUCTION, pointer);
    size_serialize(&SYSCALL_INSTRUCTION, bytes);
    list_serialize(&SYSCALL_INSTRUCTION, *list_physical_addresses, size_serialize_element);

    return 0;
}

int exit_cpu_operation(int argc, char **argv)
{

    if (argc != 1)
    {
        log_error(MODULE_LOGGER, "Uso: EXIT");
        EVICTION_REASON = UNEXPECTED_ERROR_EVICTION_REASON;
        return 1;
    }

    log_trace(MODULE_LOGGER, "EXIT");

    // Saco de la TLB
    for (int i = list_size(TLB) - 1; i >= 0; i--)
    {

        t_TLB *delete_tlb_entry = list_get(TLB, i);
        if (delete_tlb_entry->PID == EXEC_CONTEXT.PID)
        {
            list_remove(TLB, i);
        }
    }

    EXEC_CONTEXT.PC++;
    
    SYSCALL_CALLED = 1;
    cpu_opcode_serialize(&SYSCALL_INSTRUCTION, EXIT_CPU_OPCODE);

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