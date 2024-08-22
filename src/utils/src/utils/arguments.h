/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_ARGUMENTS_H
#define UTILS_ARGUMENTS_H

#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "commons/config.h"
#include "commons/string.h"
#include "commons/log.h"
#include "commons/log.h"
#include "utils/package.h"
#include "utils/module.h"

typedef struct t_Arguments {
    uint8_t argc;
    char **argv;
    const int max_argc;
} t_Arguments;


t_Arguments *arguments_create(int max_argc);
int arguments_use(t_Arguments *arguments, char *line);
void arguments_remove(t_Arguments *arguments);
void arguments_destroy(t_Arguments *arguments);
char *strip_whitespaces(char *string);

#endif // UTILS_ARGUMENTS_H