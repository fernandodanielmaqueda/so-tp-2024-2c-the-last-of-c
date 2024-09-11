/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/return_value.h"

int return_value_serialize(t_Payload *payload, t_Return_Value source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(payload_add(payload, &source, sizeof(source)))
    return -1;

  return_value_log(source);
  return 0;
}

int return_value_deserialize(t_Payload *payload, t_Return_Value *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(payload_remove(payload, destination, sizeof(t_Return_Value)))
    return -1;
    
  return_value_log(*destination);
  return 0;
}

void return_value_log(t_Return_Value source) {
  log_info(SERIALIZE_LOGGER,
    "t_Return_Value: %" PRId8
    , source
  );
}