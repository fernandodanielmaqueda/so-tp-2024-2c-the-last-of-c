/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/exec_context.h"

int str_to_tid(char *string, t_TID *destination) {
  if(string == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  char *end;

  *destination = (t_TID) strtoul(string, &end, 10);

  if(!*string || *end)
    return -1;

  return 0;
}

int str_to_pc(char *string, t_PC *destination) {
  if(string == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }
  
  char *end;

  *destination = (t_PC) strtoul(string, &end, 10);

  if(!*string || *end)
    return -1;

  return 0;
}

int str_to_priority(char *string, t_Priority *destination) {
  if(string == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  char *end;

  *destination = (t_Priority) strtoul(string, &end, 10);

  if(!*string || *end)
    return -1;

  return 0;
}

int str_to_time(char *string, t_Time *destination) {
  if(string == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  char *end;

  *destination = (t_Time) strtoul(string, &end, 10);

  if(!*string || *end)
    return -1;

  return 0;
}

int exec_context_serialize(t_Payload *payload, t_Exec_Context source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(payload_add(payload, &(source.cpu_registers.PC), sizeof(((t_Exec_Context *)0)->cpu_registers.PC)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.AX), sizeof(((t_Exec_Context *)0)->cpu_registers.AX)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.BX), sizeof(((t_Exec_Context *)0)->cpu_registers.BX)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.CX), sizeof(((t_Exec_Context *)0)->cpu_registers.CX)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.DX), sizeof(((t_Exec_Context *)0)->cpu_registers.DX)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.EX), sizeof(((t_Exec_Context *)0)->cpu_registers.EX)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.FX), sizeof(((t_Exec_Context *)0)->cpu_registers.FX)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.GX), sizeof(((t_Exec_Context *)0)->cpu_registers.GX)))
    return -1;
  if(payload_add(payload, &(source.cpu_registers.HX), sizeof(((t_Exec_Context *)0)->cpu_registers.HX)))
    return -1;
  if(payload_add(payload, &(source.base), sizeof(((t_Exec_Context *)0)->base)))
    return -1;
  if(payload_add(payload, &(source.limit), sizeof(((t_Exec_Context *)0)->limit)))
    return -1;

  exec_context_log(SERIALIZE_SERIALIZATION, source);
  return 0;
}

int exec_context_deserialize(t_Payload *payload, t_Exec_Context *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(payload_remove(payload, &(destination->cpu_registers.PC), sizeof(((t_Exec_Context *)0)->cpu_registers.PC)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.AX), sizeof(((t_Exec_Context *)0)->cpu_registers.AX)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.BX), sizeof(((t_Exec_Context *)0)->cpu_registers.BX)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.CX), sizeof(((t_Exec_Context *)0)->cpu_registers.CX)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.DX), sizeof(((t_Exec_Context *)0)->cpu_registers.DX)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.EX), sizeof(((t_Exec_Context *)0)->cpu_registers.EX)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.FX), sizeof(((t_Exec_Context *)0)->cpu_registers.FX)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.GX), sizeof(((t_Exec_Context *)0)->cpu_registers.GX)))
    return -1;
  if(payload_remove(payload, &(destination->cpu_registers.HX), sizeof(((t_Exec_Context *)0)->cpu_registers.HX)))
    return -1;
  if(payload_remove(payload, &(destination->base), sizeof(((t_Exec_Context *)0)->base)))
    return -1;
  if(payload_remove(payload, &(destination->limit), sizeof(((t_Exec_Context *)0)->limit)))
    return -1;

  exec_context_log(DESERIALIZE_SERIALIZATION, *destination);
  return 0;
}

int exec_context_log(e_Serialization serialization, t_Exec_Context source) {
  log_info(SERIALIZE_LOGGER,
    "[%s] t_Exec_Context:\n"
    "* PC: %u\n"
    "* AX: %u\n"
    "* BX: %u\n"
    "* CX: %u\n"
    "* DX: %u\n"
    "* EX: %u\n"
    "* FX: %u\n"
    "* GX: %u\n"
    "* HX: %u\n"
    "* base: %u\n"
    "* limit: %u"
    , SERIALIZATION_NAMES[serialization]
    , source.cpu_registers.PC
    , source.cpu_registers.AX
    , source.cpu_registers.BX
    , source.cpu_registers.CX
    , source.cpu_registers.DX
    , source.cpu_registers.EX
    , source.cpu_registers.FX
    , source.cpu_registers.GX
    , source.cpu_registers.HX
    , source.base
    , source.limit
    );
  
  return 0;
}