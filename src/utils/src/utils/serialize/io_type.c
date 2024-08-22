/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/io_type.h"

const char *IO_TYPE_NAMES[] = {
  [GENERIC_IO_TYPE] = "GENERIC_IO_TYPE",
  [STDIN_IO_TYPE] = "STDIN_IO_TYPE",
  [STDOUT_IO_TYPE] = "STDOUT_IO_TYPE",
  [DIALFS_IO_TYPE] = "DIALFS_IO_TYPE"
};

void io_type_serialize(t_Payload *payload, e_IO_Type source) {
  if(payload == NULL)
    return;

  t_EnumValue aux;
  
    aux = (t_EnumValue) source;
  payload_add(payload, &aux, sizeof(aux));

  io_type_log(source);
}

void io_type_deserialize(t_Payload *payload, e_IO_Type *destination) {
  if(payload == NULL || destination == NULL)
    return;

  t_EnumValue aux;
  
  payload_remove(payload, &aux, sizeof(aux));
    *destination = (e_IO_Type) aux;

  io_type_log(*destination);
}

void io_type_log(e_IO_Type source) {
  log_info(SERIALIZE_LOGGER,
    "e_IO_Type: %s"
    , IO_TYPE_NAMES[source]
  );
}