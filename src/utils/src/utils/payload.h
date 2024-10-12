/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_PAYLOAD_H
#define UTILS_PAYLOAD_H

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include "commons/log.h"

typedef uint32_t t_Size;

typedef struct t_Payload {
    size_t size;
    void *stream;
    size_t offset;
} t_Payload;

extern t_log *SERIALIZE_LOGGER;

/**
 * Inicializa el payload.
 * @param payload Puntero al payload.
 */
void payload_init(t_Payload *payload);

/**
 * Destruye el payload.
 * @param payload Puntero al payload.
 */
void payload_destroy(t_Payload *payload);

/**
 * Agrega datos al payload.
 * @param payload Puntero al payload.
 * @param source Puntero a los datos fuente.
 * @param sourceSize Tamaño en bytes de los datos fuente.
 */
int payload_add(t_Payload *payload, void *source, size_t sourceSize);


/**
 * Remueve datos del payload.
 * @param payload Puntero al payload.
 * @param destination Puntero al buffer de destino.
 * @param destinationSize Tamaño en bytes del buffer de destino.
 */
int payload_remove(t_Payload *payload, void *destination, size_t destinationSize);

/**
 * Escribe datos en el payload.
 * @param payload Puntero al payload.
 * @param source Puntero a los datos fuente.
 * @param sourceSize Tamaño en bytes de los datos fuente.
 */
int payload_write(t_Payload *payload, void *source, size_t sourceSize);

/**
 * Lee datos del payload.
 * @param payload Puntero al payload.
 * @param destination Puntero al buffer de destino.
 * @param destinationSize Tamaño en bytes del buffer de destino.
 */
int payload_read(t_Payload *payload, void *destination, size_t destinationSize);

/**
 * Mueve el offset del payload.
 * @param payload Puntero al payload.
 * @param offset Offset a mover.
 * @param whence Punto de referencia (SEEK_SET, SEEK_CUR, SEEK_END).
 */
int payload_seek(t_Payload *payload, long offset, int whence);

#endif // UTILS_PAYLOAD_H