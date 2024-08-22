/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "registers.h"

const t_CPU_Register_Info CPU_REGISTERS[] = {
    [PC_REGISTER] = {.name = "PC" , .dataType = UINT32_DATATYPE},

    [AX_REGISTER] = {.name = "AX" , .dataType = UINT8_DATATYPE},
    [BX_REGISTER] = {.name = "BX" , .dataType = UINT8_DATATYPE},
    [CX_REGISTER] = {.name = "CX" , .dataType = UINT8_DATATYPE},
    [DX_REGISTER] = {.name = "DX" , .dataType = UINT8_DATATYPE},
    [EAX_REGISTER] = {.name = "EAX" , .dataType = UINT32_DATATYPE},
    [EBX_REGISTER] = {.name = "EBX" , .dataType = UINT32_DATATYPE},
    [ECX_REGISTER] = {.name = "ECX" , .dataType = UINT32_DATATYPE},
    [EDX_REGISTER] = {.name = "EDX" , .dataType = UINT32_DATATYPE},
    [RAX_REGISTER] = {.name = "RAX" , .dataType = UINT32_DATATYPE},
    [RBX_REGISTER] = {.name = "RBX" , .dataType = UINT32_DATATYPE},
    [RCX_REGISTER] = {.name = "RCX" , .dataType = UINT32_DATATYPE},
    [RDX_REGISTER] = {.name = "RDX" , .dataType = UINT32_DATATYPE},
    [SI_REGISTER] = {.name = "SI" , .dataType = UINT32_DATATYPE},
    [DI_REGISTER] = {.name = "DI" , .dataType = UINT32_DATATYPE},
};

int decode_register(char *name, e_CPU_Register *destination) {
    if(name == NULL || destination == NULL)
        return 1;
    
    size_t cpu_registers_number = sizeof(CPU_REGISTERS) / sizeof(CPU_REGISTERS[0]);
    for (register e_CPU_Register cpu_register = 0; cpu_register < cpu_registers_number; cpu_register++)
        if (strcmp(CPU_REGISTERS[cpu_register].name, name) == 0) {
            *destination = cpu_register;
            return 0;
        }

    return 1;
}

void *get_register_pointer(t_Exec_Context *exec_context, e_CPU_Register cpu_register) {
    if(exec_context == NULL)
        return NULL;

    switch (cpu_register) {
        case PC_REGISTER:
            return (void *) &(exec_context->PC);
            
        case AX_REGISTER:
            return (void *) &(exec_context->cpu_registers.AX);
        case BX_REGISTER:
            return (void *) &(exec_context->cpu_registers.BX);
        case CX_REGISTER:
            return (void *) &(exec_context->cpu_registers.CX);
        case DX_REGISTER:
            return (void *) &(exec_context->cpu_registers.DX);
        case EAX_REGISTER:
            return (void *) &(exec_context->cpu_registers.EAX);
        case EBX_REGISTER:
            return (void *) &(exec_context->cpu_registers.EBX);
        case ECX_REGISTER:
            return (void *) &(exec_context->cpu_registers.ECX);
        case EDX_REGISTER:
            return (void *) &(exec_context->cpu_registers.EDX);
        case RAX_REGISTER:
            return (void *) &(exec_context->cpu_registers.RAX);
        case RBX_REGISTER:
            return (void *) &(exec_context->cpu_registers.RBX);
        case RCX_REGISTER:
            return (void *) &(exec_context->cpu_registers.RCX);
        case RDX_REGISTER:
            return (void *) &(exec_context->cpu_registers.RDX);
        case SI_REGISTER:
            return (void *) &(exec_context->cpu_registers.SI);
        case DI_REGISTER:
            return (void *) &(exec_context->cpu_registers.DI);
    }

    return NULL;
}

int set_register_value(t_Exec_Context *exec_context, e_CPU_Register cpu_register, uint32_t value) {
    if(exec_context == NULL)
        return 1;
        
    void *register_pointer = get_register_pointer(exec_context, cpu_register);
    if(register_pointer == NULL)
        return 1;

    switch(CPU_REGISTERS[cpu_register].dataType) {
        case UINT8_DATATYPE:
            *(uint8_t *) register_pointer = (uint8_t) value;
            break;
        case UINT32_DATATYPE:
            *(uint32_t *) register_pointer = value;
            break;
    }

    return 0;
}


int get_register_value(t_Exec_Context exec_context, e_CPU_Register cpu_register, uint32_t *destination)
{
    if(destination == NULL)
        return 1;

    void *register_pointer = get_register_pointer(&exec_context, cpu_register);
    if(register_pointer == NULL)
        return 1;

    switch(CPU_REGISTERS[cpu_register].dataType) {
        case UINT8_DATATYPE:
            *destination = (uint32_t) *((uint8_t *) register_pointer);
            break;
        case UINT32_DATATYPE:
            *destination = *((uint32_t *) register_pointer);
            break;
    }  

    return 0;
}

size_t get_register_size(e_CPU_Register cpu_register) {

    switch(CPU_REGISTERS[cpu_register].dataType) {
        case UINT8_DATATYPE:
            return sizeof(uint8_t);
        case UINT32_DATATYPE:
            return sizeof(uint32_t);
    }

    return 0;
}