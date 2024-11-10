/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_RESULT_H
#define UTILS_SERIALIZE_RESULT_H

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

typedef int8_t t_Result;
#define RESULT_MIN INT8_MIN
#define RESULT_MAX INT8_MAX


/**
 * @brief Serializacion de un t_Result para ser enviado.
 * @param payload Payload a encolar.
 * @param source t_Result fuente a serializar
 */
int result_serialize(t_Payload *payload, int source);


/**
 * @brief Deserializacion de un t_Result para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del t_Result deserializado
 */
int result_deserialize(t_Payload *payload, int *destination);


/**
 * @brief Loguea un t_Result.
 * @param result t_Result a loguear.
 */
int result_log(e_Serialization serialization, t_Result result);

#endif // UTILS_SERIALIZE_RESULT_H