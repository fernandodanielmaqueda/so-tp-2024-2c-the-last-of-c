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

void port_type_serialize(t_Payload *payload, e_Port_Type source) {
  if(payload == NULL)
    return;

  t_EnumValue aux;
  
    aux = (t_EnumValue) source;
  payload_add(payload, &aux, sizeof(aux));

  port_type_log(source);
}

void port_type_deserialize(t_Payload *payload, e_Port_Type *destination) {
  if(payload == NULL || destination == NULL)
    return;

  t_EnumValue aux;

  payload_remove(payload, &aux, sizeof(aux));
    *destination = (e_Port_Type) aux;

  port_type_log(*destination);
}

void port_type_log(e_Port_Type source) {
  log_info(SERIALIZE_LOGGER,
    "e_Port_Type: %s"
    , PORT_NAMES[source]
  );
}