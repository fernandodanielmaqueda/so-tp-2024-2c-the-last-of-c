/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/data.h"

int data_serialize(t_Payload *payload, void *data, size_t size) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }
  
  if((data == NULL) || (!size)) {
    size = 0;
    t_Size size_serialized = (t_Size) size;
    
    if(payload_add(payload, &size_serialized, sizeof(size_serialized)))
      return -1;
  } else {
    t_Size size_serialized = (t_Size) size;
    if(payload_add(payload, &size_serialized, sizeof(size_serialized)))
      return -1;
    if(payload_add(payload, data, size))
      return -1;
  }

  data_log(data, size);
  return 0;
}

int data_deserialize(t_Payload *payload, void **data, size_t *size) {
  if(payload == NULL || data == NULL || size == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_Size size_serialized;
  
  if(payload_remove(payload, &size_serialized, sizeof(size_serialized)))
    return -1;

  if(!size_serialized) {
    *data = NULL;
    *size = 0;
  } else {

    *data = malloc((size_t) size_serialized);
    if(*data == NULL) {
      log_warning(SERIALIZE_LOGGER, "malloc: No se pudieron reservar %zu bytes para deserializar los datos", (size_t) size_serialized);
      errno = ENOMEM;
      return -1;
    }

    if(payload_remove(payload, *data, (size_t) size_serialized)) {
      free(*data);
      return -1;
    }

    *size = (size_t) size_serialized;

  }

  data_log(*data, (size_t) size_serialized);
  return 0;
}

void data_log(void *data, size_t size) {
  log_info(SERIALIZE_LOGGER,
    "data:\n"
    "* stream: %p\n"
    "* size: %zu"
    , data
    , size
  );
}