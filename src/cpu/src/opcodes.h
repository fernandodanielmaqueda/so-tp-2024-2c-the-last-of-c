/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef CPU_OPCODES_H
#define CPU_OPCODES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/memory.h"
#include "commons/bitarray.h"
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "utils/module.h"
#include "utils/arguments.h"
#include "utils/socket.h"
#include "socket.h"
#include "cpu.h"

typedef struct t_CPU_Operation {
    int (*function) (int, char**);
} t_CPU_Operation;

extern t_CPU_Operation CPU_OPERATIONS[];

int decode_instruction(char *name, e_CPU_OpCode *cpu_opcode);
int set_cpu_operation(int argc, char **argv);
int read_mem_cpu_operation(int argc, char **argv);
int write_mem_cpu_operation(int argc, char **argv);
int sum_cpu_operation(int argc, char **argv);
int sub_cpu_operation(int argc, char **argv);
int jnz_cpu_operation(int argc, char **argv);
int log_cpu_operation(int argc, char **argv);
int exit_cpu_operation(int argc, char **argv);
int str_to_uint32(char *string, uint32_t *destination);
int str_to_size(char *string, size_t *destination);
int str_to_pc(char *string, t_PC *destination);

#endif // CPU_OPCODES_H