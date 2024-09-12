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

typedef struct t_Bitmap {
    t_bitarray * bits_blocks; // puntero a al bitarray
    uint32_t bits_free; // contar los bits libres (0)
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

extern t_bitarray *BITMAP;
extern char *PTRO_BLOCKS;
extern size_t BLOCKS_TOTAL_SIZE;



int module(int, char*[]);
void initialize_global_variables(void);
void finish_global_variables(void);
void read_module_config(t_config *module_config);
//int io_type_find(char *name, e_IO_Type *destination);
void initialize_sockets(void);
void finish_sockets(void);
void *filesystem_client_handler_for_memory(t_Client *new_client);
uint32_t seek_first_free_block();
t_FS_File* seek_file(char* file_name);
bool can_assign_block(size_t initial_position, size_t len, size_t final_len);
size_t seek_quantity_blocks_required(size_t puntero, size_t bytes);
void initialize_blocks();
void free_bitmap_blocks();
void create_file(char* file_name, size_t first_block);
void update_file(char* file_name, size_t size, size_t location);
int quantity_free_blocks();
void compact_blocks(t_FS_File* file, size_t qBlocks, size_t newSize);
t_FS_File* seek_file_by_header_index(size_t position);
void moveBlock(size_t blocks_to_move, size_t free_spaces, size_t location);
//----
t_Bitmap* create_bitmap(); 
bool exist_free_bits_bitmap(t_Bitmap* bit_map, uint32_t count_block_demand);

#endif // FILESYSTEM_H