/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_SUBHEADER_H
#define UTILS_SERIALIZE_SUBHEADER_H

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
 * @brief Serializacion de un e_Header para ser enviado.
 * @param payload Payload a encolar.
 * @param source e_Header fuente a serializar
 */
int subheader_serialize(t_Payload *payload, e_Header source);


/**
 * @brief Deserializacion de un e_Header para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del e_Header deserializado
 */
int subheader_deserialize(t_Payload *payload, e_Header *destination);


/**
 * @brief Loguea un e_Header.
 * @param subheader e_Header a loguear.
 */
void subheader_log(e_Header source);

#endif // UTILS_SERIALIZE_SUBHEADER_H