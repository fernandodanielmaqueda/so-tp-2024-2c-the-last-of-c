/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_SUBPAYLOAD_H
#define UTILS_SERIALIZE_SUBPAYLOAD_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "utils/package.h"
#include "utils/module.h"
#include "utils/serialize/data.h"


/**
 * @brief Serializacion de un t_Payload para ser enviado.
 * @param payload Payload a encolar.
 * @param source t_Payload fuente a serializar
 */
int subpayload_serialize(t_Payload *payload, t_Payload source);


/**
 * @brief Deserializacion de un t_Payload para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del t_Payload deserializado
 */
int subpayload_deserialize(t_Payload *payload, t_Payload *destination);


/**
 * @brief Loguea un t_Payload.
 * @param subpayload t_Payload a loguear.
 */
void subpayload_log(t_Payload source);

#endif // UTILS_SERIALIZE_SUBPAYLOAD_H