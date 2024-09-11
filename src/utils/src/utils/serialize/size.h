/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_SIZE_H
#define UTILS_SERIALIZE_SIZE_H

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


int str_to_uint32(char *string, uint32_t *destination);


int str_to_size(char *string, size_t *destination);


int size_serialize_element(t_Payload *payload, size_t *source);


int size_deserialize_element(t_Payload *payload, size_t **destination);


/**
 * @brief Serializacion de un size_t para ser enviado.
 * @param payload Payload a encolar.
 * @param source size_t fuente a serializar
 */
int size_serialize(t_Payload *payload, size_t source);


/**
 * @brief Deserializacion de un size_t para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del size_t deserializado
 */
int size_deserialize(t_Payload *payload, size_t *destination);


/**
 * @brief Loguea un size_t.
 * @param source size_t a loguear.
 */
void size_log(size_t source);

#endif // UTILS_SERIALIZE_SIZE_H