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

typedef int64_t t_Quantum;

typedef struct t_CPU_Registers {
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t RAX;
    uint32_t RBX;
    uint32_t RCX;
    uint32_t RDX;
    uint32_t SI;
    uint32_t DI;
} t_CPU_Registers;

typedef struct t_Exec_Context {
    t_PID PID;
    t_PC PC;
    t_CPU_Registers cpu_registers;
} t_Exec_Context;


/**
 * @brief Serializacion de un t_Exec_Context para ser enviado.
 * @param payload Payload a encolar.
 * @param source t_Exec_Context fuente a serializar
 */
void exec_context_serialize(t_Payload *payload, t_Exec_Context source);


/**
 * @brief Deserializacion de un t_Exec_Context para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del t_Exec_Context deserializado
 */
void exec_context_deserialize(t_Payload *payload, t_Exec_Context *destination);


/**
 * @brief Loguea un t_Exec_Context.
 * @param source t_Exec_Context a loguear.
 */
void exec_context_log(t_Exec_Context source);

#endif // UTILS_SERIALIZE_EXEC_CONTEXT_H