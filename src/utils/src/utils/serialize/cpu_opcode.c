/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/cpu_opcode.h"

const char *CPU_OPCODE_NAMES[] = {
    [SET_CPU_OPCODE] = "SET",
    [MOV_IN_CPU_OPCODE] = "MOV_IN",
    [MOV_OUT_CPU_OPCODE] = "MOV_OUT",
    [SUM_CPU_OPCODE] = "SUM",
    [SUB_CPU_OPCODE] = "SUB",
    [JNZ_CPU_OPCODE] = "JNZ",
    [RESIZE_CPU_OPCODE] = "RESIZE",
    [COPY_STRING_CPU_OPCODE] = "COPY_STRING",
    [WAIT_CPU_OPCODE] = "WAIT",
    [SIGNAL_CPU_OPCODE] = "SIGNAL",
    [IO_GEN_SLEEP_CPU_OPCODE] = "IO_GEN_SLEEP",
    [IO_STDIN_READ_CPU_OPCODE] = "IO_STDIN_READ",
    [IO_STDOUT_WRITE_CPU_OPCODE] = "IO_STDOUT_WRITE",
    [IO_FS_CREATE_CPU_OPCODE] = "IO_FS_CREATE",
    [IO_FS_DELETE_CPU_OPCODE] = "IO_FS_DELETE",
    [IO_FS_TRUNCATE_CPU_OPCODE] = "IO_FS_TRUNCATE",
    [IO_FS_WRITE_CPU_OPCODE] = "IO_FS_WRITE",
    [IO_FS_READ_CPU_OPCODE] = "IO_FS_READ",
    [EXIT_CPU_OPCODE] = "EXIT",
    NULL
};

void cpu_opcode_serialize(t_Payload *payload, e_CPU_OpCode source) {
  if(payload == NULL)
    return;

  t_EnumValue aux;
  
    aux = (t_EnumValue) source;
  payload_add(payload, &aux, sizeof(aux));

  cpu_opcode_log(source);
}

void cpu_opcode_deserialize(t_Payload *payload, e_CPU_OpCode *destination) {
  if(payload == NULL || destination == NULL)
    return;

  t_EnumValue aux;
  
  payload_remove(payload, &aux, sizeof(aux));
    *destination = (e_CPU_OpCode) aux;

  cpu_opcode_log(*destination);
}

void cpu_opcode_log(e_CPU_OpCode cpu_opcode) {
  log_info(SERIALIZE_LOGGER,
    "e_CPU_OpCode: %s"
    , CPU_OPCODE_NAMES[cpu_opcode]
  );
}