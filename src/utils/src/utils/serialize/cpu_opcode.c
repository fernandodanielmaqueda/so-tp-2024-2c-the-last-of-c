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
    [PROCESS_CREATE_CPU_OPCODE] = "PROCESS_CREATE",
    [PROCESS_EXIT_CPU_OPCODE] = "PROCESS_EXIT",
    [THREAD_CREATE_CPU_OPCODE] = "THREAD_CREATE",
    [THREAD_JOIN_CPU_OPCODE] = "THREAD_JOIN",
    [THREAD_CANCEL_CPU_OPCODE] = "THREAD_CANCEL",
    [THREAD_EXIT_CPU_OPCODE] = "THREAD_EXIT",
    [MUTEX_CREATE_CPU_OPCODE] = "MUTEX_CREATE",
    [MUTEX_LOCK_CPU_OPCODE] = "MUTEX_LOCK",
    [MUTEX_UNLOCK_CPU_OPCODE] = "MUTEX_UNLOCK",
    [DUMP_MEMORY_CPU_OPCODE] = "DUMP_MEMORY",
    [IO_CPU_OPCODE] = "IO",
    NULL
};

int cpu_opcode_serialize(t_Payload *payload, e_CPU_OpCode source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_EnumValue aux;
  aux = (t_EnumValue) source;
  
  if(payload_add(payload, &aux, sizeof(aux))) {
    return -1;
  }

  cpu_opcode_log(SERIALIZE_SERIALIZATION, source);
  return 0;
}

int cpu_opcode_deserialize(t_Payload *payload, e_CPU_OpCode *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_EnumValue aux;

  if(payload_remove(payload, &aux, sizeof(aux)))
    return -1;

  *destination = (e_CPU_OpCode) aux;

  cpu_opcode_log(DESERIALIZE_SERIALIZATION, *destination);
  return 0;
}

int cpu_opcode_log(e_Serialization serialization, e_CPU_OpCode cpu_opcode) {
  log_trace_r(&SERIALIZE_LOGGER,
    "[%s] e_CPU_OpCode: %s"
    , SERIALIZATION_NAMES[serialization]
    , CPU_OPCODE_NAMES[cpu_opcode]
  );

  return 0;
}