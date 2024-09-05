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


typedef struct t_Memory_Register {
    uint32_t PC;
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
    uint32_t EX;
    uint32_t FX; 
    uint32_t GX; 
    uint32_t HX;
} t_Memory_Register;
typedef struct t_Process {
    t_PID PID; //Puede ser duplicado
    t_PID TID; //Unico
    t_list *instructions_list;
    t_Memory_Register registers;
    uint32_t base;
    uint32_t limit;
    t_list *tid_list;
} t_Process;

typedef struct t_Page {
    size_t page_number;
    size_t assigned_frame_number;
    bool bit_uso;
    bool bit_modificado;
    bool bit_presencia;
    time_t last_use;
} t_Page;

typedef struct t_Frame {
    size_t frame_number;
    t_PID PID;
    t_Page *assigned_page;
} t_Frame;


int module(int, char*[]);
void initialize_mutexes(void);
void finish_mutexes(void);
void initialize_semaphores(void);
void finish_semaphores(void);
void read_module_config(t_config *module_config);

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

void listen_io(t_Client *client);

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
 * @brief
 * @param process
 * @param pid
 */
bool process_matches_pid(t_Process *process, t_PID *pid);



/**
 * @brief Funcion que encapsula al hilo escucha cpu
 * @param socket Socket escuchado
 */
void listen_cpu(void);


/**
 * @brief Funcion que encapsula al hilo escucha kernel
 * @param socket Socket escuchado
 */
void listen_kernel(void);

/**
 * @brief Crea los marcos e inicializa la lista de los mismos
 */
void create_frames(void);

/**
 * @brief Libera el espacio reservado para los marcos
 */
void free_frames(void);


/**
 * @brief Recibe el pedido de busqueda de marco y responde el mismo
 * @param socketRecibido Socket escuchado
 */
void respond_frame_request(t_Payload *socketRecibido);

/**
 * @brief Busca el marco asociado a una pagina en especial de una tabla de paginas.
 * @param tablaPaginas Tanla de paginas del proceso donde buscar la pagina.
 * @param pagina Pagina buscada.
 */
size_t *seek_frame_number_by_page_number(t_list *tablaPaginas, size_t page_number);

void resize_process(t_Payload *payload);
void write_memory(t_Payload *socketRecibido, int socket);
void read_memory(t_Payload *socketRecibido, int socket);
void update_page(size_t frame_number);
int get_next_dir_fis(size_t frame_number, t_PID pid);
int seek_oldest_page_updated(t_list *page_list);
void free_memory();
void free_all_process();

void io_write_memory(t_Payload *payload, int socket);
void io_read_memory(t_Payload *payload, int socket);
void copy_memory(t_Payload *payload, int socket);

#endif /* MEMORIA_H */