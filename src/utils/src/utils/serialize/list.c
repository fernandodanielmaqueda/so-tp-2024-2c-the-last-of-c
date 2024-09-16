/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/serialize/list.h"

int list_serialize(t_Payload *payload, t_list source, int (*element_serializer)(t_Payload *, void *)) {
  if(payload == NULL || element_serializer == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_Size list_size = (t_Size) source.elements_count;
  if(payload_add(payload, &list_size, sizeof(list_size)))
    return -1;

  t_link_element *element = source.head;
  for(; list_size > 0; list_size--) {

    if(element_serializer(payload, element->data))
      return -1;

    element = element->next;
  }

  list_log(source);
  return 0;
}

int list_deserialize(t_Payload *payload, t_list *destination, int (*element_deserializer)(t_Payload *, void **)) {
  if(payload == NULL || destination == NULL || element_deserializer == NULL) {
    errno = EINVAL;
    return -1;
  }

  t_Size list_size;

  if(payload_remove(payload, &list_size, sizeof(list_size)))
    return -1;

  t_link_element *new_element, **last_element = &(destination->head);
  for(; list_size > 0; list_size--) {
    new_element = malloc(sizeof(t_link_element));
    if(new_element == NULL) {
      log_warning(SERIALIZE_LOGGER, "malloc: No se pudieron reservar %zu bytes para deserializar un elemento de la lista", sizeof(t_link_element));
      errno = ENOMEM;
      return -1;
    }

    if(element_deserializer(payload, &(new_element->data))) {
      free(new_element);
      return -1;
    }

    new_element->next = NULL;

    *last_element = new_element;
    destination->elements_count++;
    
    last_element = &(new_element->next);
  }

  list_log(*destination);
  return 0;
}

int list_log(t_list list) {
  char *string_elements = string_new();
  if(string_elements == NULL) {
    log_warning(SERIALIZE_LOGGER, "string_new: No se pudo reservar memoria para loguear los elementos de la lista");
    errno = ENOMEM;
    return -1;
  }

  t_link_element *element = list.head;
  for(register int i = 0; i < list.elements_count; i++) {
    string_append_with_format(&string_elements, "* element (%d/%d): %p\n", i, list.elements_count, element->data);    
    element = element->next;
  }

  log_info(SERIALIZE_LOGGER,
    "t_list:\n"
    "* elements_count: %d\n"
    "* head: %p\n"
    "%s"
    , list.elements_count
    , (void *) list.head
    , string_elements
  );

  free(string_elements);

  return 0;
}