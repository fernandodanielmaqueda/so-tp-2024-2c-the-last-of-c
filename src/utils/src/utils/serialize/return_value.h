/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_RETURN_VALUE_H
#define UTILS_SERIALIZE_RETURN_VALUE_H

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

typedef int8_t t_Return_Value;


/**
 * @brief Serializacion de un t_Return_Value para ser enviado.
 * @param payload Payload a encolar.
 * @param source t_Return_Value fuente a serializar
 */
void return_value_serialize(t_Payload *payload, t_Return_Value source);


/**
 * @brief Deserializacion de un t_Return_Value para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del t_Return_Value deserializado
 */
void return_value_deserialize(t_Payload *payload, t_Return_Value *destination);


/**
 * @brief Loguea un t_Return_Value.
 * @param return_value t_Return_Value a loguear.
 */
void return_value_log(t_Return_Value return_value);

#endif // UTILS_SERIALIZE_RETURN_VALUE_H