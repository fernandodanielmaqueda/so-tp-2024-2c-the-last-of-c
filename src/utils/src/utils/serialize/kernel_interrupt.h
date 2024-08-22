/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_KERNEL_INTERRUPT_H
#define UTILS_SERIALIZE_KERNEL_INTERRUPT_H

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

typedef enum e_Kernel_Interrupt {
    NONE_KERNEL_INTERRUPT,
    QUANTUM_KERNEL_INTERRUPT,
    KILL_KERNEL_INTERRUPT
} e_Kernel_Interrupt;

extern const char *KERNEL_INTERRUPT_NAMES[];

/**
 * @brief Serializacion de un e_Kernel_Interrupt para ser enviado.
 * @param payload Payload a encolar.
 * @param source e_Kernel_Interrupt fuente a serializar
 */
void kernel_interrupt_serialize(t_Payload *payload, e_Kernel_Interrupt source);


/**
 * @brief Deserializacion de un e_Kernel_Interrupt para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del e_Kernel_Interrupt deserializado
 */
void kernel_interrupt_deserialize(t_Payload *payload, e_Kernel_Interrupt *destination);


/**
 * @brief Loguea un e_Kernel_Interrupt.
 * @param kernel_interrupt e_Kernel_Interrupt a loguear.
 */
void kernel_interrupt_log(e_Kernel_Interrupt kernel_interrupt);

#endif // UTILS_SERIALIZE_KERNEL_INTERRUPT_H