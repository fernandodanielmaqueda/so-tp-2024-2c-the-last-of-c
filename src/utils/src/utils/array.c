
/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/array.h"

// array_create
// array_init (&)

t_Array *array_create(size_t element_size, size_t max) {
    t_Array *array = malloc(sizeof(t_Array));
    if(array == NULL) {
        return NULL;
    }

    array->stream = NULL;
    array->size = 0;
    array->element_size = element_size;
    array->max = max;

    return array;
}