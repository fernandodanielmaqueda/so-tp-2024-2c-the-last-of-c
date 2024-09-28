/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/result.h"

int result_serialize(t_Payload *payload, int source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(source < RESULT_MIN || source > RESULT_MAX) {
    errno = ERANGE;
    return -1;
  }

  t_Result source_serialized = (t_Result) source;
  if(payload_add(payload, &source_serialized, sizeof(source_serialized)))
    return -1;

  result_log(source_serialized);
  return 0;
}

int result_deserialize(t_Payload *payload, int *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_Result destination_serialized;
  if(payload_remove(payload, &destination_serialized, sizeof(destination_serialized)))
    return -1;
  
  *destination = (int) destination_serialized;
    
  result_log(destination_serialized);
  return 0;
}

int result_log(t_Result source) {
  log_info(SERIALIZE_LOGGER,
    "t_Result: %d"
    , source
  );

  return 0;
}