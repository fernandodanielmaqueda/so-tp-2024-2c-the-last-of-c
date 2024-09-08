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

typedef uint32_t t_Partition_Number;

typedef struct t_Partition {
    size_t size; // Tamaño de la partición
    size_t base; // Offset/Desplazamiento respecto del inicio de la memoria

    bool occupied;
    t_PID pid; // Opcional: Gasto un poco de memoria para hacer más efeciente la búsqueda de qué proceso está ocupando una partición en particular. Capaz está al pedo si la consigna no lo pide :))    
    size_t limit; // Esto es por la fragmentación interna: que el tamaño de un proceso puede ser menor que el tamaño de la partición
} t_Partition;

typedef struct t_Memory_Thread {
    t_Exec_Context exec_context;

    t_PC instructions_count;
    char **array_instructions;
} t_Memory_Thread;

typedef struct t_Memory_Process {
    t_Partition_Number partition_number;

    t_TID tid_count;
    t_Memory_Thread **array_memory_threads;
    pthread_mutex_t mutex_array_memory_threads;
} t_Memory_Process;

extern char *MODULE_NAME;

extern t_log *MODULE_LOGGER;
extern char *MODULE_LOG_PATHNAME;

extern t_config *MODULE_CONFIG;
extern char *MODULE_CONFIG_PATHNAME;

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

void read_module_config(t_config *module_config);

int memory_management_scheme_find(char *name, e_Memory_Management_Scheme *destination);

int memory_allocation_algorithm_find(char *name, e_Memory_Allocation_Algorithm *destination);

/**
 * @brief Busca el archivo de pseudocodigo y crea la estructura dentro de memoria
 * @param socketRecibido Socket desde donde se va a recibir el pcb.
 */
void create_process(t_Payload *payload);

/**
 * @brief Elimina el proceso, marca el marco como disponible y libera la pagina
 * @param socketRecibido Socket desde donde se va a recibir el pcb.
 */
void kill_process (t_Payload *payload);

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
int parse_pseudocode_file(char *path, t_list *list_instruction);

/**
 * @brief Funcion que encapsula al hilo escucha cpu
 * @param socket Socket escuchado
 */
void listen_cpu(void);

/**
 * @brief Funcion que encapsula al hilo escucha kernel
 * @param socket Socket escuchado
 */
void *listen_kernel(t_Client *new_client);

void attend_write(t_Payload *socketRecibido, int socket);

void attend_read(t_Payload *socketRecibido, int socket);

void free_memory();

#endif // MEMORIA_H