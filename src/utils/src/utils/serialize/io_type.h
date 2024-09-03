/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_IO_TYPE_H
#define UTILS_SERIALIZE_IO_TYPE_H

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

/* 
typedef enum e_IO_Type {
    GENERIC_IO_TYPE,
    STDIN_IO_TYPE,
    STDOUT_IO_TYPE,
    DIALFS_IO_TYPE
} e_IO_Type;

*/

//extern const char *IO_TYPE_NAMES[];

/**
 * @brief Serializacion de un e_IO_Type para ser enviado.
 * @param payload Payload a encolar.
 * @param source e_IO_Type fuente a serializar
 */
//void io_type_serialize(t_Payload *payload, e_IO_Type io_type);


/**
 * @brief Deserializacion de un e_IO_Type para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del e_IO_Type deserializado
 */
//void io_type_deserialize(t_Payload *payload, e_IO_Type *io_type);


/**
 * @brief Loguea un e_IO_Type.
 * @param io_type e_IO_Type a loguear.
 */
//void io_type_log(e_IO_Type io_type);

#endif // UTILS_SERIALIZE_IO_TYPE_H