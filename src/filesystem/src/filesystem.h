/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include "commons/bitarray.h"
#include "utils/module.h"
#include "utils/send.h"
#include "utils/socket.h"
#include "utils/package.h"
#include "socket.h"

// typedef struct t_FS_File {
//     char *name;

//     uint32_t initial_bloq;
//     uint32_t len;
//     uint32_t size;
// } t_FS_File;

typedef uint32_t t_Block_Pointer; //tipo array de bloques para copiar 

typedef struct t_Bitmap {
    t_bitarray *bits_blocks; // puntero a al bitarray
    size_t blocks_free; // contar los bits libres (0)
} t_Bitmap;

/*
    t_Bitmap* create_t_bitmap(): 
            crear esta estructura y arrancar en cero, free_bits =  cantida de bloques

    bool exist_free_bits(count_block):  
            si tiene disponible la cantida de bloques (count_block + 1)  

    void  set_bits(): 
            
        recorrer el bitarray seteando los indices (cero a uno)
        guardar sus posiciones (nro indice) en lista. 
        Sincroniza el archivo.
        ejecutar write_block(datos*, list_index):  hacer en mas un hilo?? Escribir el archivo de Block.dat
        liberar mutex  
        
*/

extern t_Server SERVER_FILESYSTEM;

extern char *MOUNT_DIR;
extern size_t BLOCK_SIZE;
extern size_t BLOCK_COUNT;
extern int BLOCK_ACCESS_DELAY;

extern FILE *FILE_BLOCKS;
extern FILE *FILE_METADATA;
extern char *PTRO_BITMAP;
extern size_t BITMAP_SIZE;

extern t_Bitmap BITMAP;
extern pthread_mutex_t MUTEX_BITMAP;

extern void *PTRO_BLOCKS;
extern size_t BLOCKS_TOTAL_SIZE;

#define BLOCKS_TOTAL_SIZE (BLOCK_COUNT * BLOCK_SIZE) // cantidad de bloques * tamaño de bloque

int module(int, char*[]);

int initialize_global_variables(void);
int finish_global_variables(void);

int read_module_config(t_config *module_config);

int bitmap_init();
int bloques_init(void); // void ** &PTRO_BLOCKS

void filesystem_client_handler_for_memory(int fd_client);

void set_bits_bitmap(t_Bitmap *bit_map, t_Block_Pointer *array, size_t blocks_necessary, char* filename);
bool exist_free_bits_bitmap(t_Bitmap* bit_map, uint32_t count_block_demand);

size_t necessary_bits(size_t bytes_size);
void* get_pointer_to_block(void *file_ptr, size_t file_block_size, t_Block_Pointer file_block_pos) ;
void* get_pointer_to_block_from_file(t_Block_Pointer file_block_pos);
void block_msync(t_Block_Pointer block_number);
void write_block(t_Block_Pointer nro_bloque, void* ptro_datos, size_t desplazamiento);
void create_metadata_file(const char *filename, size_t size, t_Block_Pointer index_block) ;
void read_block(t_Block_Pointer nro_bloque, void* ptro_datos, size_t desplazamiento);

bool is_address_in_mapped_area(void *addr) ;	

void create_directory(const char *path);
#endif // FILESYSTEM_H