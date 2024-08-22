/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SERIALIZE_CPU_INSTRUCTION_H
#define UTILS_SERIALIZE_CPU_INSTRUCTION_H

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

typedef uint32_t t_Value;

typedef enum e_CPU_OpCode {
    SET_CPU_OPCODE,
    MOV_IN_CPU_OPCODE,
    MOV_OUT_CPU_OPCODE,
    SUM_CPU_OPCODE,
    SUB_CPU_OPCODE,
    JNZ_CPU_OPCODE,
    RESIZE_CPU_OPCODE,
    COPY_STRING_CPU_OPCODE,
    WAIT_CPU_OPCODE,
    SIGNAL_CPU_OPCODE,
    IO_GEN_SLEEP_CPU_OPCODE,
    IO_STDIN_READ_CPU_OPCODE,
    IO_STDOUT_WRITE_CPU_OPCODE,
    IO_FS_CREATE_CPU_OPCODE,
    IO_FS_DELETE_CPU_OPCODE,
    IO_FS_TRUNCATE_CPU_OPCODE,
    IO_FS_WRITE_CPU_OPCODE,
    IO_FS_READ_CPU_OPCODE,
    EXIT_CPU_OPCODE
} e_CPU_OpCode;

extern const char *CPU_OPCODE_NAMES[];

/**
 * @brief Serializacion de un e_CPU_OpCode para ser enviado.
 * @param payload Payload a encolar.
 * @param source e_CPU_OpCode fuente a serializar
 */
void cpu_opcode_serialize(t_Payload *payload, e_CPU_OpCode cpu_opcode);


/**
 * @brief Deserializacion de un e_CPU_OpCode para ser leido.
 * @param payload Payload a desencolar.
 * @param destination Destino del e_CPU_OpCode deserializado
 */
void cpu_opcode_deserialize(t_Payload *payload, e_CPU_OpCode *cpu_opcode);


/**
 * @brief Loguea un e_CPU_OpCode.
 * @param cpu_opcode e_CPU_OpCode a loguear.
 */
void cpu_opcode_log(e_CPU_OpCode cpu_opcode);

#endif // UTILS_SERIALIZE_CPU_INSTRUCTION_H