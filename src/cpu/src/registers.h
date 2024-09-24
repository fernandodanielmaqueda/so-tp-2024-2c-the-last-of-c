/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include <stdio.h>
#include <string.h>
#include <utils/serialize/exec_context.h>

typedef enum e_CPU_Register {
    PC_REGISTER,
	AX_REGISTER,
	BX_REGISTER,
	CX_REGISTER,
	DX_REGISTER,
    EX_REGISTER,
    FX_REGISTER,
    GX_REGISTER,
    HX_REGISTER    
} e_CPU_Register;

typedef enum e_CPU_Register_DataType {
    UINT32_DATATYPE
} e_CPU_Register_DataType;

typedef struct t_CPU_Register_Info {
    char *name;
    e_CPU_Register_DataType dataType;
} t_CPU_Register_Info;

extern const t_CPU_Register_Info CPU_REGISTERS[];

int decode_register(char *name, e_CPU_Register *destination);
void *get_register_pointer(t_Exec_Context *exec_context, e_CPU_Register cpu_register);
int set_register_value(t_Exec_Context *exec_context, e_CPU_Register cpu_register, uint32_t value);
int get_register_value(t_Exec_Context exec_context, e_CPU_Register cpu_register, uint32_t *destination);
size_t get_register_size(e_CPU_Register cpu_register);