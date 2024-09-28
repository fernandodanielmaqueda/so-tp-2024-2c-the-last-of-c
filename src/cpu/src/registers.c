/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "registers.h"

const t_CPU_Register_Info CPU_REGISTERS[] = {
    [PC_REGISTER] = {.name = "PC" , .dataType = UINT32_DATATYPE},
    [AX_REGISTER] = {.name = "AX" , .dataType = UINT32_DATATYPE},
    [BX_REGISTER] = {.name = "BX" , .dataType = UINT32_DATATYPE},
    [CX_REGISTER] = {.name = "CX" , .dataType = UINT32_DATATYPE},
    [DX_REGISTER] = {.name = "DX" , .dataType = UINT32_DATATYPE},
    [EX_REGISTER] = {.name = "EX" , .dataType = UINT32_DATATYPE},
    [FX_REGISTER] = {.name = "FX" , .dataType = UINT32_DATATYPE},
    [GX_REGISTER] = {.name = "GX" , .dataType = UINT32_DATATYPE},
    [HX_REGISTER] = {.name = "HX" , .dataType = UINT32_DATATYPE}
};

int decode_register(char *name, e_CPU_Register *destination) {
    if(name == NULL || destination == NULL)
        return -1;
    
    size_t cpu_registers_number = sizeof(CPU_REGISTERS) / sizeof(CPU_REGISTERS[0]);
    for (register e_CPU_Register cpu_register = 0; cpu_register < cpu_registers_number; cpu_register++)
        if(strcmp(CPU_REGISTERS[cpu_register].name, name) == 0) {
            *destination = cpu_register;
            return 0;
        }

    return -1;
}

void *get_register_pointer(t_Exec_Context *exec_context, e_CPU_Register cpu_register) {
    if(exec_context == NULL)
        return NULL;

    switch (cpu_register) {
        case PC_REGISTER:
            return (void *) &(exec_context->cpu_registers.PC);
        case AX_REGISTER:
            return (void *) &(exec_context->cpu_registers.AX);
        case BX_REGISTER:
            return (void *) &(exec_context->cpu_registers.BX);
        case CX_REGISTER:
            return (void *) &(exec_context->cpu_registers.CX);
        case DX_REGISTER:
            return (void *) &(exec_context->cpu_registers.DX);
        case EX_REGISTER:
            return (void *) &(exec_context->cpu_registers.EX);
        case FX_REGISTER:
            return (void *) &(exec_context->cpu_registers.FX);
        case GX_REGISTER:
            return (void *) &(exec_context->cpu_registers.GX);
        case HX_REGISTER:
            return (void *) &(exec_context->cpu_registers.HX);
        
    }

    return NULL;
}

int set_register_value(t_Exec_Context *exec_context, e_CPU_Register cpu_register, uint32_t value) {
    if(exec_context == NULL)
        return -1;
        
    void *register_pointer = get_register_pointer(exec_context, cpu_register);
    if(register_pointer == NULL)
        return -1;

    switch(CPU_REGISTERS[cpu_register].dataType) {
       
        case UINT32_DATATYPE:
            *(uint32_t *) register_pointer = value;
            break;
    }

    return 0;
}


int get_register_value(t_Exec_Context exec_context, e_CPU_Register cpu_register, uint32_t *destination)
{
    if(destination == NULL)
        return -1;

    void *register_pointer = get_register_pointer(&exec_context, cpu_register);
    if(register_pointer == NULL)
        return -1;

    switch(CPU_REGISTERS[cpu_register].dataType) {
        
        case UINT32_DATATYPE:
            *destination = *((uint32_t *) register_pointer);
            break;
    }  

    return 0;
}

size_t get_register_size(e_CPU_Register cpu_register) {

    switch(CPU_REGISTERS[cpu_register].dataType) {
       
        case UINT32_DATATYPE:
            return sizeof(uint32_t);
    }

    return 0;
}