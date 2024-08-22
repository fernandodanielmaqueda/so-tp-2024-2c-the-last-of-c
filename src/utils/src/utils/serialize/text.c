/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/text.h"

void text_serialize(t_Payload *payload, char *source) {
  if(payload == NULL)
    return;
  
  t_Size textLength;

  if(source == NULL) {
    textLength = 0;
    payload_add(payload, (void *) &(textLength), sizeof(textLength));
  } else {
    textLength = (t_Size) strlen(source) + 1;
    payload_add(payload, (void *) &(textLength), sizeof(textLength));
    payload_add(payload, (void *) source, (size_t) textLength);
  }

  text_log(source);
}

void text_deserialize(t_Payload *payload, char **destination) {
  if(payload == NULL || destination == NULL)
    return;

  t_Size textLength;
  payload_remove(payload, (void *) &(textLength), sizeof(textLength));

  if(!textLength) {
    *destination = NULL;
  } else {
    *destination = malloc((size_t) textLength);
    if(*destination == NULL) {
      log_error(SERIALIZE_LOGGER, "No se pudo reservar memoria para la cadena de destino");
      exit(EXIT_FAILURE);
    }
    
    payload_remove(payload, (void *) *destination, (size_t) textLength);
  }

  text_log(*destination);
}

void text_log(char *text) {

  log_info(SERIALIZE_LOGGER,
    "text: %s"
    , (text != NULL) ? text : "(nil)"
    );
}