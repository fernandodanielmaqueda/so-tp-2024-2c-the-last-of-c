/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_EXEC_CONTEXT_H
#define UTILS_SERIALIZE_EXEC_CONTEXT_H

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

typedef uint16_t t_PID;
#define PID_MAX UINT16_MAX

typedef uint16_t t_TID;
#define TID_MAX UINT16_MAX

typedef uint32_t t_PC;

typedef unsigned int t_Priority;

typedef int64_t t_Time;

typedef struct t_CPU_Registers {
    t_PC PC;
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
    uint32_t EX;
    uint32_t FX;
    uint32_t GX;
    uint32_t HX;
} t_CPU_Registers;

typedef struct t_Exec_Context {
    t_CPU_Registers cpu_registers;
    uint32_t base;
    uint32_t limit;
} t_Exec_Context;


int str_to_tid(char *string, t_TID *destination);


int str_to_pc(char *string, t_PC *destination);


int str_to_priority(char *string, t_Priority *destination);


int str_to_time(char *string, t_Time *destination);

/**
 * @brief Serializacion de un t_Exec_Context para ser enviado.
 * @param payload Payload a encolar.
 * @param source t_Exec_Context fuente a serializar
 */
int exec_context_serialize(t_Payload *payload, t_Exec_Context source);


/**
 * @brief Deserializacion de un t_Exec_Context para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del t_Exec_Context deserializado
 */
int exec_context_deserialize(t_Payload *payload, t_Exec_Context *destination);


/**
 * @brief Loguea un t_Exec_Context.
 * @param source t_Exec_Context a loguear.
 */
int exec_context_log(t_Exec_Context source);

#endif // UTILS_SERIALIZE_EXEC_CONTEXT_H