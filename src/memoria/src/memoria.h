/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef MEMORIA_H
#define MEMORIA_H

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/memory.h"
#include "commons/bitarray.h"
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "utils/module.h"
#include "utils/arguments.h"
#include "utils/send.h"
#include "utils/socket.h"

typedef enum e_Memory_Management_Scheme {
    FIXED_PARTITIONING_MEMORY_MANAGEMENT_SCHEME,
    DYNAMIC_PARTITIONING_MEMORY_MANAGEMENT_SCHEME
} e_Memory_Management_Scheme;

typedef struct t_Memory_Management_Scheme {
    char *name;
    void (*function)(void);
} t_Memory_Management_Scheme;

typedef enum e_Memory_Allocation_Algorithm {
    FIRST_FIT_MEMORY_ALLOCATION_ALGORITHM,
    BEST_FIT_MEMORY_ALLOCATION_ALGORITHM,
    WORST_FIT_MEMORY_ALLOCATION_ALGORITHM
} e_Memory_Allocation_Algorithm;

typedef struct t_Memory_Allocation_Algorithm {
    char *name;
    void (*function)(void);
} t_Memory_Allocation_Algorithm;

typedef struct t_Partition {
    size_t size; // Tamaño de la partición
    size_t base; // Desplazamiento/Offset
    pthread_rwlock_t rwlock_partition; // Mutex de lectura/escritura

    bool occupied;
    t_PID pid; // PID del proceso que la ocupa (TODO: ¿Podría ser un t_Memory_Process*?)
} t_Partition;

typedef struct t_Memory_Thread {
    t_TID tid;
    t_CPU_Registers registers;
    t_PC instructions_count;
    char **array_instructions;
} t_Memory_Thread;

typedef struct t_Memory_Process {
    t_PID pid;
    size_t size;
    t_Partition *partition;

    t_TID tid_count;
    t_Memory_Thread **array_memory_threads;
    pthread_rwlock_t rwlock_array_memory_threads;
} t_Memory_Process;

extern size_t MEMORY_SIZE;
extern char *INSTRUCTIONS_PATH;
extern int RESPONSE_DELAY;

extern t_Memory_Management_Scheme MEMORY_MANAGEMENT_SCHEMES[];
extern e_Memory_Management_Scheme MEMORY_MANAGEMENT_SCHEME;

extern t_Memory_Allocation_Algorithm MEMORY_ALLOCATION_ALGORITHMS[];
extern e_Memory_Allocation_Algorithm MEMORY_ALLOCATION_ALGORITHM;

extern void *MAIN_MEMORY;

extern pthread_rwlock_t RWLOCK_PARTITIONS_AND_PROCESSES;

extern t_list *PARTITION_TABLE;

extern t_PID PID_COUNT;
extern t_Memory_Process **ARRAY_PROCESS_MEMORY;

#include "client_kernel.h"
#include "client_cpu.h"
#include "socket.h"

int module(int, char*[]);

int partition_destroy(t_Partition *partition);

int partition_table_destroy(void);

int read_module_config(t_config *module_config);

int memory_management_scheme_find(char *name, e_Memory_Management_Scheme *destination);

int memory_allocation_algorithm_find(char *name, e_Memory_Allocation_Algorithm *destination);

void free_memory();

void free_threads(t_Memory_Process *process);

#endif // MEMORIA_H