/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_TEXT_H
#define UTILS_SERIALIZE_TEXT_H

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
 * @brief Serializacion de un texto para ser enviado.
 * @param payload Payload a encolar.
 * @param source Texto fuente a serializar
 */
int text_serialize(t_Payload *payload, char *source);


/**
 * @brief Deserializacion de un texto para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del texto deserializado
 */
int text_deserialize(t_Payload *payload, char **destination);


/**
 * @brief Loguea un texto.
 * @param text Texto a loguear.
 */
int text_log(char *text);

#endif // UTILS_SERIALIZE_TEXT_H