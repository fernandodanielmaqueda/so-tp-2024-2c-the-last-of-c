/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/size.h"

int str_to_uint32(char *string, uint32_t *destination)
{
    char *end;

    *destination = (uint32_t) strtoul(string, &end, 10);

    if(!*string || *end)
        return 1;
        
    return 0;
}

int str_to_size(char *string, size_t *destination)
{
    char *end;

    *destination = (size_t) strtoul(string, &end, 10);

    if(!*string || *end)
        return 1;
        
    return 0;
}

void size_serialize_element(t_Payload *payload, void *source) {
    if(payload == NULL || source == NULL)
        return;

    size_serialize(payload, *(size_t *) source);
}

void size_deserialize_element(t_Payload *payload, void **destination) {
  if(payload == NULL || destination == NULL)
    return;

  *destination = malloc(sizeof(size_t));
  if(*destination == NULL) {
    log_error(SERIALIZE_LOGGER, "malloc: No se pudo reservar memoria para size_t");
    exit(EXIT_FAILURE);
  }

  size_deserialize(payload, (size_t *) *destination);
}

void size_serialize(t_Payload *payload, size_t source) {
  if(payload == NULL)
    return;

  t_Size aux;
  
    aux = (t_Size) source;
  payload_add(payload, &aux, sizeof(aux));

  size_log(source);
}

void size_deserialize(t_Payload *payload, size_t *destination) {
  if(payload == NULL || destination == NULL)
    return;

  t_Size aux;

  payload_remove(payload, &aux, sizeof(aux));
    *destination = (size_t) aux;

  size_log(*destination);
}

void size_log(size_t source) {
  log_info(SERIALIZE_LOGGER,
    "size_t: %zd"
    , source
  );
}