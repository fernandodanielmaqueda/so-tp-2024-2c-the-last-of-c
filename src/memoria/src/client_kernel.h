/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef MEMORY_CLIENT_KERNEL_H
#define MEMORY_CLIENT_KERNEL_H

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <commons/temporal.h>
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
#include "memoria.h"

/**
 * @brief Funcion que encapsula al hilo escucha kernel
 * @param socket Socket escuchado
 */
void listen_kernel(int fd_client);

/**
 * @brief Busca el archivo de pseudocodigo y crea la estructura dentro de memoria
 * @param socketRecibido Socket desde donde se va a recibir el pcb.
 */
void attend_process_create(int fd_client, t_Payload *payload);

/**
 * @brief Elimina el proceso, marca el marco como disponible y libera la pagina
 * @param socketRecibido Socket desde donde se va a recibir el pcb.
 */
void attend_process_destroy(int fd_client, t_Payload *payload);

void attend_thread_create(int fd_client, t_Payload *payload);

void attend_thread_destroy(int fd_client, t_Payload *payload);

void attend_dump_memory(int fd_client, t_Payload *payload);

void allocate_partition(t_Partition **partition, size_t *index_partition, size_t required_size);

int split_partition(size_t index_partition, size_t size);

int add_element_to_array_process(t_Memory_Process *process);

int verify_and_join_splited_partitions(t_Partition *partition);

/**
 * @brief Busca un archivo, lo lee y crea una lista de instrucciones
 * @param path Path donde se encuentra el archivo.
 * @param list_instruction Lista a llenarse con las instrucciones del archivo.
 */
int parse_pseudocode_file(char *argument_path, t_Memory_Thread *new_thread);


#endif // MEMORY_CLIENT_KERNEL_H