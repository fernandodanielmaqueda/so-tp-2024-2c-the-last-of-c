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
#include "socket.h"

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

// typedef uint32_t t_Partition_Number;

typedef struct t_Partition {
    t_PID pid;
    size_t base;
    size_t size; // AKA:: LIMIT --> Tamaño de la partición
    bool occupied;
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
   // t_Partition_Number partition_number;
    t_Partition* partition;
    t_TID tid_count;
    t_Memory_Thread **array_memory_threads;
    pthread_mutex_t mutex_array_memory_threads;
} t_Memory_Process;

typedef struct t_FS_Data{
        t_PID pid;
        t_TID tid;
        char* namefile;
        void *position;
} t_FS_Data;

extern size_t MEMORY_SIZE;
extern char *INSTRUCTIONS_PATH;
extern int RESPONSE_DELAY;

extern t_Memory_Management_Scheme MEMORY_MANAGEMENT_SCHEMES[];
extern e_Memory_Management_Scheme MEMORY_MANAGEMENT_SCHEME;

extern t_Memory_Allocation_Algorithm MEMORY_ALLOCATION_ALGORITHMS[];
extern e_Memory_Allocation_Algorithm MEMORY_ALLOCATION_ALGORITHM;

extern void *MAIN_MEMORY;

extern t_list *PARTITION_TABLE;
extern pthread_mutex_t MUTEX_PARTITION_TABLE;

extern t_PID PID_COUNT;
extern t_Memory_Process **ARRAY_PROCESS_MEMORY;
extern pthread_mutex_t MUTEX_ARRAY_PROCESS_MEMORY;

int module(int, char*[]);

void initialize_global_variables(void);

void finish_global_variables(void);

int read_module_config(t_config *module_config);

int memory_management_scheme_find(char *name, e_Memory_Management_Scheme *destination);

int memory_allocation_algorithm_find(char *name, e_Memory_Allocation_Algorithm *destination);

/**
 * @brief Busca el archivo de pseudocodigo y crea la estructura dentro de memoria
 * @param socketRecibido Socket desde donde se va a recibir el pcb.
 */
int create_process(t_Payload *payload);

/**
 * @brief Elimina el proceso, marca el marco como disponible y libera la pagina
 * @param socketRecibido Socket desde donde se va a recibir el pcb.
 */
int kill_process (t_Payload *payload);

/**
 * @brief Busca la lista de instruccion y devuelve la instruccion buscada
 * @param pid Program counter requerido.
 * @param pc Program counter requerido.
 */
void seek_instruccion(t_Payload *socketRecibido);

/**
 * @brief Crea la lista de instrucciones asociada al archivo pasado por parametro
 * @param file Archivo a leer
 * @param list_instruction Lista a llenarse con las instrucciones del archivo.
 */
void create_instruction(FILE *file, t_list *list_instruction);

/**
 * @brief Busca un archivo, lo lee y crea una lista de instrucciones
 * @param path Path donde se encuentra el archivo.
 * @param list_instruction Lista a llenarse con las instrucciones del archivo.
 */
int parse_pseudocode_file(char *path, char*** array_instruction, t_PC* count);

/**
 * @brief Funcion que encapsula al hilo escucha cpu
 * @param socket Socket escuchado
 */
void listen_cpu(void);

/**
 * @brief Funcion que encapsula al hilo escucha kernel
 * @param socket Socket escuchado
 */
void listen_kernel(int fd_client);

int write_memory(t_Payload *socketRecibido);

int read_memory(t_Payload *socketRecibido, int socket);

void free_memory();

int split_partition(int position, size_t size);
int add_element_to_array_process (t_Memory_Process* process);
int verify_and_join_splited_partitions(t_PID pid);
void free_threads(int pid);
int create_thread(t_Payload *payload);
int kill_thread(t_Payload *payload);
int treat_memory_dump(t_Payload *payload);
void* attend_memory_dump(void* arg);
void seek_cpu_context(t_Payload *payload);
void update_cpu_context(t_Payload *payload);


#endif // MEMORIA_H