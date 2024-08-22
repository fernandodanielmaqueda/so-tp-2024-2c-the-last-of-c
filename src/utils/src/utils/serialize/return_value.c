/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/return_value.h"

void return_value_serialize(t_Payload *payload, t_Return_Value source) {
  if(payload == NULL)
    return;

  payload_add(payload, &source, sizeof(source));

  return_value_log(source);
}

void return_value_deserialize(t_Payload *payload, t_Return_Value *destination) {
  if(payload == NULL || destination == NULL)
    return;

  payload_remove(payload, destination, sizeof(*destination));

  return_value_log(*destination);
}

void return_value_log(t_Return_Value source) {
  log_info(SERIALIZE_LOGGER,
    "t_Return_Value: %" PRId8
    , source
  );
}