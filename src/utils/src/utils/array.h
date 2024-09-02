/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_ARRAY_H
#define UTILS_ARRAY_H

#include <stdlib.h>

typedef struct t_Array {
    void *stream;
    size_t size;
    size_t element_size;
    size_t max;
} t_Array;



#endif // UTILS_ARRAY_H