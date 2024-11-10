/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_DATA_H
#define UTILS_SERIALIZE_DATA_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "commons/config.h"
#include "commons/memory.h"
#include "commons/string.h"
#include "utils/package.h"

#include "commons/log.h"
#include "utils/module.h"


/**
 * @brief Serializacion de datos para ser enviados.
 * @param payload Payload a encolar.
 * @param data Datos a serializar.
 * @param bytes Cantidad de bytes a serializar.
 */
int data_serialize(t_Payload *payload, void *data, size_t bytes);


/**
 * @brief Deserializacion de datos para ser leidos.
 * @param payload Payload a desencolar.
 * @param data Datos deserializados.
 * @param bytes Cantidad de bytes a deserializar.
 */
int data_deserialize(t_Payload *payload, void **data, size_t *bytes);


/**
 * @brief Loguea un texto.
 * @param text Texto a loguear.
 */
int data_log(e_Serialization serialization, void *data, size_t bytes);

#endif // UTILS_SERIALIZE_DATA_H