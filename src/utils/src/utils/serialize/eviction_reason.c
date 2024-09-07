/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/eviction_reason.h"

const char *EVICTION_REASON_NAMES[] = {
    [UNEXPECTED_ERROR_EVICTION_REASON] = "UNEXPECTED_ERROR_EVICTION_REASON",
    [SEGMENTATION_FAULT_EVICTION_REASON] = "SEGMENTATION_FAULT_EVICTION_REASON",
    [EXIT_EVICTION_REASON] = "EXIT_EVICTION_REASON",
    [KILL_KERNEL_INTERRUPT_EVICTION_REASON] = "KILL_KERNEL_INTERRUPT_EVICTION_REASON",
    [SYSCALL_EVICTION_REASON] = "SYSCALL_EVICTION_REASON",
    [QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON] = "QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON"  
};

void eviction_reason_serialize(t_Payload *payload, e_Eviction_Reason source) {
  if(payload == NULL)
    return;

  t_EnumValue aux;
  
    aux = (t_EnumValue) source;
  payload_add(payload, &aux, sizeof(aux));

  eviction_reason_log(source);
}

void eviction_reason_deserialize(t_Payload *payload, e_Eviction_Reason *destination) {
  if(payload == NULL || destination == NULL)
    return;

  t_EnumValue aux;

  payload_remove(payload, &aux, sizeof(aux));
    *destination = (e_Eviction_Reason) aux;

  eviction_reason_log(*destination);
}

void eviction_reason_log(e_Eviction_Reason eviction_reason) {
  log_info(SERIALIZE_LOGGER,
    "e_Eviction_Reason: %s"
    , EVICTION_REASON_NAMES[eviction_reason]
  );
}