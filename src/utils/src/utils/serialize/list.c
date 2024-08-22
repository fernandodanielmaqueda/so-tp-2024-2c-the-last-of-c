/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/list.h"

void list_serialize(t_Payload *payload, t_list source, void (*element_serializer)(t_Payload *, void *)) {
  if(payload == NULL || element_serializer == NULL)
    return;

  t_Size list_size = (t_Size) source.elements_count;
  payload_add(payload, (void *) &list_size, sizeof(list_size));

  t_link_element *element = source.head;
  for(; list_size > 0; list_size--) {
    element_serializer(payload, element->data);
    element = element->next;
  }

  list_log(source);
}

void list_deserialize(t_Payload *payload, t_list *destination, void (*element_deserializer)(t_Payload *, void **)) {
  if(payload == NULL || destination == NULL || element_deserializer == NULL)
    return;

  t_Size list_size;
  payload_remove(payload, (void *) &list_size, sizeof(list_size));
  destination->elements_count = (int) list_size;

  t_link_element *new_element, **last_element = &(destination->head);
  for(; list_size > 0; list_size--) {
    new_element = malloc(sizeof(t_link_element));
    if(new_element == NULL) {
      log_error(SERIALIZE_LOGGER, "No se pudo reservar memoria para el nuevo elemento de la lista");
      exit(EXIT_FAILURE);
    }

    element_deserializer(payload, &(new_element->data));
    new_element->next = NULL;

    *last_element = new_element;
    last_element = &(new_element->next);
  }

  list_log(*destination);
}

void list_log(t_list list) {

  log_info(SERIALIZE_LOGGER,
    "t_list:\n"
    "* elements_count: %d"
    , list.elements_count
  );
}