/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/kernel_interrupt.h"

const char *KERNEL_INTERRUPT_NAMES[] = {
  [NONE_KERNEL_INTERRUPT] = "NONE_KERNEL_INTERRUPT",
  [QUANTUM_KERNEL_INTERRUPT] = "QUANTUM_KERNEL_INTERRUPT",
  [KILL_KERNEL_INTERRUPT] = "KILL_KERNEL_INTERRUPT"
};

int kernel_interrupt_serialize(t_Payload *payload, e_Kernel_Interrupt source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_EnumValue aux;
  aux = (t_EnumValue) source;

  if(payload_add(payload, &aux, sizeof(aux)))
    return -1;

  kernel_interrupt_log(SERIALIZE_SERIALIZATION, source);
  return 0;
}

int kernel_interrupt_deserialize(t_Payload *payload, e_Kernel_Interrupt *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_EnumValue aux;
  
  if(payload_remove(payload, &aux, sizeof(aux)))
    return -1;

  *destination = (e_Kernel_Interrupt) aux;
  
  kernel_interrupt_log(DESERIALIZE_SERIALIZATION, *destination);
  return 0;
}

int kernel_interrupt_log(e_Serialization serialization, e_Kernel_Interrupt source) {
  log_trace_r(&SERIALIZE_LOGGER,
    "[%s] e_Kernel_Interrupt: %s"
    , SERIALIZATION_NAMES[serialization]
    , KERNEL_INTERRUPT_NAMES[source]
  );

  return 0;
}