/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/subpayload.h"

int subpayload_serialize(t_Payload *payload, t_Payload source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(data_serialize(payload, source.stream, source.size))
    return -1;

  subpayload_log(SERIALIZE_SERIALIZATION, source);

  return 0;
}

int subpayload_deserialize(t_Payload *payload, t_Payload *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(data_deserialize(payload, &(destination->stream), &(destination->size)))
    return -1;

  subpayload_log(DESERIALIZE_SERIALIZATION, *destination);
  return 0;
}

int subpayload_log(e_Serialization serialization, t_Payload source) {
  char *dump_string = mem_hexstring(source.stream, source.size);

  log_info_r(&SERIALIZE_LOGGER,
    "[%s] t_Payload:\n"
    "* size: %zd\n"
    "* stream: %p\n"
    "%s"
    , SERIALIZATION_NAMES[serialization]
    , source.size
    , source.stream
    , dump_string
  );

  free(dump_string);

  return 0;
}