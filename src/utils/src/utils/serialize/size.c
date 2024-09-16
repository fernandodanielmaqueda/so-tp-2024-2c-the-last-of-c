/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/size.h"

int str_to_uint32(char *string, uint32_t *destination)
{
  if(string == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  char *end;

  *destination = (uint32_t) strtoul(string, &end, 10);

  if(!*string || *end)
    return -1;
      
  return 0;
}

int str_to_size(char *string, size_t *destination)
{
  if(string == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  char *end;

  *destination = (size_t) strtoul(string, &end, 10);

  if(!*string || *end)
    return -1;
      
  return 0;
}

int size_serialize_element(t_Payload *payload, size_t *source) {
  if(payload == NULL || source == NULL) {
    errno = EINVAL;
    return -1;
  }

  if(size_serialize(payload, *source))
    return -1;

  return 0;
}

int size_deserialize_element(t_Payload *payload, size_t **destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  *destination = malloc(sizeof(size_t));
  if(*destination == NULL) {
    log_warning(SERIALIZE_LOGGER, "malloc: No se pudieron reservar %zu bytes para deserializar un size_t", sizeof(size_t));
    errno = ENOMEM;
    return -1;
  }

  if(size_deserialize(payload, *destination)) {
    free(*destination);
    return -1;
  }

  return 0;
}

int size_serialize(t_Payload *payload, size_t source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }
  
  t_Size size_serialized = (t_Size) source;

  if(payload_add(payload, &size_serialized, sizeof(size_serialized)))
    return -1;

  size_log(source);
  return 0;
}

int size_deserialize(t_Payload *payload, size_t *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_Size size_serialized;

  if(payload_remove(payload, &size_serialized, sizeof(size_serialized)))
    return -1;
  
  *destination = (size_t) size_serialized;

  size_log(*destination);
  return 0;
}

void size_log(size_t source) {
  log_info(SERIALIZE_LOGGER,
    "size_t: %zu"
    , source
  );
}