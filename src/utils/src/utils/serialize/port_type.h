/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_PORT_TYPE_H
#define UTILS_SERIALIZE_PORT_TYPE_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "commons/config.h"
#include "commons/string.h"
#include "utils/package.h"
#include "utils/module.h"
#include "commons/log.h"

typedef enum e_Port_Type {
    KERNEL_PORT_TYPE,
    KERNEL_CPU_DISPATCH_PORT_TYPE,
    KERNEL_CPU_INTERRUPT_PORT_TYPE,
    CPU_PORT_TYPE,
    CPU_DISPATCH_PORT_TYPE,
    CPU_INTERRUPT_PORT_TYPE,
    MEMORY_PORT_TYPE,
    FILESYSTEM_PORT_TYPE,
    TO_BE_IDENTIFIED_PORT_TYPE
} e_Port_Type;

extern const char *PORT_NAMES[];

/**
 * @brief Serializacion de un e_Port_Type para ser enviado.
 * @param payload Payload a encolar.
 * @param source e_Port_Type fuente a serializar
 */
int port_type_serialize(t_Payload *payload, e_Port_Type port_type);


/**
 * @brief Deserializacion de un e_Port_Type para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del e_Port_Type deserializado
 */
int port_type_deserialize(t_Payload *payload, e_Port_Type *port_type);


/**
 * @brief Loguea un e_Port_Type.
 * @param port_type e_Port_Type a loguear.
 */
int port_type_log(e_Port_Type port_type);

#endif // UTILS_SERIALIZE_PORT_TYPE_H