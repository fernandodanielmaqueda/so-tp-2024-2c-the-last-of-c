/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/cpu_opcode.h"

const char *CPU_OPCODE_NAMES[] = {
    [SET_CPU_OPCODE] = "SET",
    [READ_MEM_CPU_OPCODE] = "READ_MEM",
    [WRITE_MEM_CPU_OPCODE] = "WRITE_MEM",
    [SUM_CPU_OPCODE] = "SUM",
    [SUB_CPU_OPCODE] = "SUB",
    [JNZ_CPU_OPCODE] = "JNZ",
    [LOG_CPU_OPCODE] = "LOG",
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