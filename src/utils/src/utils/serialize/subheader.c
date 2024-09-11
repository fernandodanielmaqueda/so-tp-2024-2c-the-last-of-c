/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/subheader.h"

int subheader_serialize(t_Payload *payload, e_Header source) {
  if(payload == NULL) {
    errno = EINVAL;
    return 1;
  }

  t_EnumValue aux;
  
  aux = (t_EnumValue) source;
  
  if(payload_add(payload, &aux, sizeof(aux)))
    return 1;

  subheader_log(source);
  return 0;
}

int subheader_deserialize(t_Payload *payload, e_Header *destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return 1;
  }

  t_EnumValue aux;
  
  if(payload_remove(payload, &aux, sizeof(aux)))
    return 1;

  *destination = (e_Header) aux;
  
  subheader_log(*destination);
  return 0;
}

void subheader_log(e_Header source) {
  log_info(SERIALIZE_LOGGER,
    "e_Header: %s"
    , HEADER_NAMES[source]
  );
}