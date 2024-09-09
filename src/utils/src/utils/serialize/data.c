/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/data.h"

void data_serialize(t_Payload *payload, void *data, size_t size) {
  if(payload == NULL)
    return;
  
  if((data == NULL) || (!size)) {
    size = 0;
    t_Size size_serialized = (t_Size) size;
    payload_add(payload, &size_serialized, sizeof(size_serialized));
  } else {
    t_Size size_serialized = (t_Size) size;
    payload_add(payload, &size_serialized, sizeof(size_serialized));
    payload_add(payload, data, size);
  }

  data_log(data, size);
}

void data_deserialize(t_Payload *payload, void **data, size_t *size) {
  if(payload == NULL)
    return;

  t_Size size_serialized;
  payload_remove(payload, &size_serialized, sizeof(size_serialized));

  if(!size_serialized) {

    if(data != NULL)
      *data = NULL;

    if(size != NULL)
      *size = 0;

  } else {

    if(data != NULL) {
      *data = malloc((size_t) size_serialized);
      if(*data == NULL) {
        log_error(SERIALIZE_LOGGER, "No se pudo reservar memoria para los datos");
        exit(EXIT_FAILURE);
      }
    }

    payload_remove(payload, *data, (size_t) size_serialized);

    if(size != NULL)
      *size = (size_t) size_serialized;

  }

  if(data != NULL)
    data_log(*data, (size_t) size_serialized);
  else
    data_log(NULL, (size_t) size_serialized);

}

void data_log(void *data, size_t size) {
  log_info(SERIALIZE_LOGGER,
    "data: %p - %zu bytes"
    , data
    , size
  );
}