/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/exec_context.h"

void exec_context_serialize(t_Payload *payload, t_Exec_Context source) {
  if(payload == NULL)
    return;

  payload_add(payload, &(source.PID), sizeof(source.PID));
  payload_add(payload, &(source.PC), sizeof(source.PC));
  payload_add(payload, &(source.quantum), sizeof(source.quantum));
  payload_add(payload, &(source.cpu_registers.AX), sizeof(source.cpu_registers.AX));
  payload_add(payload, &(source.cpu_registers.BX), sizeof(source.cpu_registers.BX));
  payload_add(payload, &(source.cpu_registers.CX), sizeof(source.cpu_registers.CX));
  payload_add(payload, &(source.cpu_registers.DX), sizeof(source.cpu_registers.DX));
  payload_add(payload, &(source.cpu_registers.EAX), sizeof(source.cpu_registers.EAX));
  payload_add(payload, &(source.cpu_registers.EBX), sizeof(source.cpu_registers.EBX));
  payload_add(payload, &(source.cpu_registers.ECX), sizeof(source.cpu_registers.ECX));
  payload_add(payload, &(source.cpu_registers.EDX), sizeof(source.cpu_registers.EDX));
  payload_add(payload, &(source.cpu_registers.RAX), sizeof(source.cpu_registers.RAX));
  payload_add(payload, &(source.cpu_registers.RBX), sizeof(source.cpu_registers.RBX));
  payload_add(payload, &(source.cpu_registers.RCX), sizeof(source.cpu_registers.RCX));
  payload_add(payload, &(source.cpu_registers.RDX), sizeof(source.cpu_registers.RDX));
  payload_add(payload, &(source.cpu_registers.SI), sizeof(source.cpu_registers.SI));
  payload_add(payload, &(source.cpu_registers.DI), sizeof(source.cpu_registers.DI));

  exec_context_log(source);
}

void exec_context_deserialize(t_Payload *payload, t_Exec_Context *destination) {
  if(payload == NULL || destination == NULL)
    return;

  payload_remove(payload, &(destination->PID), sizeof(destination->PID));
  payload_remove(payload, &(destination->PC), sizeof(destination->PC));
  payload_remove(payload, &(destination->quantum), sizeof(destination->quantum));
  payload_remove(payload, &(destination->cpu_registers.AX), sizeof(destination->cpu_registers.AX));
  payload_remove(payload, &(destination->cpu_registers.BX), sizeof(destination->cpu_registers.BX));
  payload_remove(payload, &(destination->cpu_registers.CX), sizeof(destination->cpu_registers.CX));
  payload_remove(payload, &(destination->cpu_registers.DX), sizeof(destination->cpu_registers.DX));
  payload_remove(payload, &(destination->cpu_registers.EAX), sizeof(destination->cpu_registers.EAX));
  payload_remove(payload, &(destination->cpu_registers.EBX), sizeof(destination->cpu_registers.EBX));
  payload_remove(payload, &(destination->cpu_registers.ECX), sizeof(destination->cpu_registers.ECX));
  payload_remove(payload, &(destination->cpu_registers.EDX), sizeof(destination->cpu_registers.EDX));
  payload_remove(payload, &(destination->cpu_registers.RAX), sizeof(destination->cpu_registers.RAX));
  payload_remove(payload, &(destination->cpu_registers.RBX), sizeof(destination->cpu_registers.RBX));
  payload_remove(payload, &(destination->cpu_registers.RCX), sizeof(destination->cpu_registers.RCX));
  payload_remove(payload, &(destination->cpu_registers.RDX), sizeof(destination->cpu_registers.RDX));
  payload_remove(payload, &(destination->cpu_registers.SI), sizeof(destination->cpu_registers.SI));
  payload_remove(payload, &(destination->cpu_registers.DI), sizeof(destination->cpu_registers.DI));

  exec_context_log(*destination);
}

void exec_context_log(t_Exec_Context source) {
  log_info(SERIALIZE_LOGGER,
    "t_Exec_Context:\n"
    "* PID: %" PRIu32 "\n"
    "* PC: %" PRIu32 "\n"
    "* quantum: %" PRId64 "\n"
    "* AX: %" PRIu8 "\n"
    "* BX: %" PRIu8 "\n"
    "* CX: %" PRIu8 "\n"
    "* DX: %" PRIu8 "\n"
    "* EAX: %" PRIu32 "\n"
    "* EBX: %" PRIu32 "\n"
    "* ECX: %" PRIu32 "\n"
    "* EDX: %" PRIu32 "\n"
    "* RAX: %" PRIu32 "\n"
    "* RBX: %" PRIu32 "\n"
    "* RCX: %" PRIu32 "\n"
    "* RDX: %" PRIu32 "\n"
    "* SI: %" PRIu32 "\n"
    "* DI: %" PRIu32
    , source.PID
    , source.PC
    , source.quantum
    , source.cpu_registers.AX
    , source.cpu_registers.BX
    , source.cpu_registers.CX
    , source.cpu_registers.DX
    , source.cpu_registers.EAX
    , source.cpu_registers.EBX
    , source.cpu_registers.ECX
    , source.cpu_registers.EDX
    , source.cpu_registers.RAX
    , source.cpu_registers.RBX
    , source.cpu_registers.RCX
    , source.cpu_registers.RDX
    , source.cpu_registers.SI
    , source.cpu_registers.DI
    );
}