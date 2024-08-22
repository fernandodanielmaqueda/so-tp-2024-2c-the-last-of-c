/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_LIST_H
#define UTILS_SERIALIZE_LIST_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "commons/config.h"
#include "commons/string.h"
#include "utils/package.h"

#include "commons/log.h"
#include "utils/module.h"


/**
 * @brief Serializacion de un t_list para ser enviado.
 * @param payload Payload a encolar.
 * @param source t_list fuente a serializar
 * @param element_serializer Funcion que serializa un elemento de la lista.
 */
void list_serialize(t_Payload *payload, t_list source, void (*element_serializer)(t_Payload *, void *));


/**
 * @brief Deserializacion de un t_list para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del t_list deserializado
 * @param element_deserializer Funcion que deserializa un elemento de la lista.
 */
void list_deserialize(t_Payload *payload, t_list *destination, void (*element_deserializer)(t_Payload *, void **));


/**
 * @brief Loguea un t_list.
 * @param list t_list a loguear.
 */
void list_log(t_list list);

#endif // UTILS_SERIALIZE_LIST_H