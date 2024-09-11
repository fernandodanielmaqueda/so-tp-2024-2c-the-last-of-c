/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/port_type.h"

const char *PORT_NAMES[] = {
  [KERNEL_PORT_TYPE] = "Kernel",
  [CPU_PORT_TYPE] = "CPU",
  [CPU_DISPATCH_PORT_TYPE] = "CPU (Dispatch)",
  [CPU_INTERRUPT_PORT_TYPE] = "CPU (Interrupt)",
  [MEMORY_PORT_TYPE] = "Memoria",
  [FILESYSTEM_PORT_TYPE] = "Filesystem",
  [TO_BE_IDENTIFIED_PORT_TYPE] = "A identificar"
};

int port_type_serialize(t_Payload *payload, e_Port_Type source) {
  if(payload == NULL) {
    errno = EINVAL;
    return 1;
  }

  t_EnumValue aux;
  aux = (t_EnumValue) source;

  if(payload_add(payload, &aux, sizeof(aux)))
    return 1;

  port_type_log(source);
  return 0;
}

int port_type_deserialize(t_Payload *payload, e_Port_Type *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return 1;
  }

  t_EnumValue aux;

  if(payload_remove(payload, &aux, sizeof(aux)))
    return 1;

  *destination = (e_Port_Type) aux;
  
  port_type_log(*destination);
  return 0;
}

void port_type_log(e_Port_Type source) {
  log_info(SERIALIZE_LOGGER,
    "e_Port_Type: %s"
    , PORT_NAMES[source]
  );
}