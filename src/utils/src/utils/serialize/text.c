/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/text.h"

int text_serialize(t_Payload *payload, char *source) {
  if(payload == NULL) {
    errno = EINVAL;
    return -1;
  }
  
  t_Size textLength;

  if(source == NULL) {
    textLength = 0;

    if(payload_add(payload, &(textLength), sizeof(textLength)))
      return -1;

  } else {
    textLength = (t_Size) strlen(source) + 1;
    
    if(payload_add(payload, &(textLength), sizeof(textLength)))
      return -1;
    if(payload_add(payload, source, (size_t) textLength))
      return -1;
  }

  text_log(source);
  return 0;
}

int text_deserialize(t_Payload *payload, char **destination) {
  if(payload == NULL || destination == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_Size textLength;
  
  if(payload_remove(payload, &(textLength), sizeof(textLength)))
    return -1;

  if(!textLength) {
    *destination = NULL;
  } else {
    *destination = malloc((size_t) textLength);
    if(*destination == NULL) {
      errno = ENOMEM;
      return -1;
    }

    if(payload_remove(payload, *destination, (size_t) textLength)) {
      free(*destination);
      return -1;
    }
  }

  text_log(*destination);
  return 0;
}

void text_log(char *text) {

  log_info(SERIALIZE_LOGGER,
    "text: %s"
    , (text != NULL) ? text : "(nil)"
    );
}