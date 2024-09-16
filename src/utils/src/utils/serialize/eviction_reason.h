/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_EVICTION_REASON_H
#define UTILS_SERIALIZE_EVICTION_REASON_H

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

typedef enum e_Eviction_Reason {
    UNEXPECTED_ERROR_EVICTION_REASON,
    SEGMENTATION_FAULT_EVICTION_REASON,
    EXIT_EVICTION_REASON, // Exclusivamente para las syscalls PROCESS_EXIT y THREAD_EXIT
    KILL_KERNEL_INTERRUPT_EVICTION_REASON,
    SYSCALL_EVICTION_REASON, // Excepto para las syscalls PROCESS_EXIT y THREAD_EXIT
    QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON
} e_Eviction_Reason;

extern const char *EVICTION_REASON_NAMES[];

/**
 * @brief Serializacion de un e_Eviction_Reason para ser enviado.
 * @param payload Payload a encolar.
 * @param source e_Eviction_Reason fuente a serializar
 */
int eviction_reason_serialize(t_Payload *payload, e_Eviction_Reason source);


/**
 * @brief Deserializacion de un e_Eviction_Reason para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del e_Eviction_Reason deserializado
 */
int eviction_reason_deserialize(t_Payload *payload, e_Eviction_Reason *destination);


/**
 * @brief Loguea un e_Eviction_Reason.
 * @param eviction_reason e_Eviction_Reason a loguear.
 */
int eviction_reason_log(e_Eviction_Reason eviction_reason);

#endif // UTILS_SERIALIZE_EVICTION_REASON_H