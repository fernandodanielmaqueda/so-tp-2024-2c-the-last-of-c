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

typedef uint32_t t_Block_Pointer;
// sizeof(t_Block_Pointer) == 8 bytes

typedef struct t_Bitmap {
    t_bitarray *bitarray; // Puntero al array de bits
    size_t size; // Tamaño del bitmap en bytes
    size_t free_blocks; // Contador de bloques libres
} t_Bitmap;

extern t_Server SERVER_FILESYSTEM;

extern char *MOUNT_DIR;
extern size_t BLOCK_SIZE;
extern size_t BLOCK_COUNT;
extern int BLOCK_ACCESS_DELAY;

extern FILE *FILE_BLOCKS;
extern char *PTRO_BITMAP;
extern size_t BITMAP_SIZE;

extern t_Bitmap BITMAP;
extern pthread_mutex_t MUTEX_BITMAP;

extern void *PTRO_BLOCKS;

//#undef MODULE_NAME
//#define MODULE_NAME "Filesystem"

//#undef MODULE_CONFIG_PATHNAME
//#define MODULE_CONFIG_PATHNAME "filesystem.config"

#undef MODULE_LOGGER_PATHNAME
#define MODULE_LOGGER_PATHNAME "filesystem.log"

#undef MODULE_LOGGER_NAME
#define MODULE_LOGGER_NAME "Filesystem"

#define FILESYSTEM_SIZE (BLOCK_COUNT * BLOCK_SIZE) // Cantidad de bloques * Tamaño de bloque

#define BITS_TO_BYTES(bits) (((bits) + 7) / 8)

int module(int, char*[]);

int read_module_config(t_config *module_config);

void make_directories(void);
int create_directory(const char *path);

int bitmap_init();
int bloques_init(void); // void ** &PTRO_BLOCKS

void filesystem_client_handler_for_memory(int fd_client);

void set_bits_bitmap(t_Bitmap *bit_map, t_Block_Pointer *array, size_t blocks_necessary, char* filename);
//bool exist_free_bits_bitmap(t_Bitmap* bit_map, uint32_t count_block_demand);

void *get_pointer_to_memory(void * memory_ptr, size_t memory_partition_size, t_Block_Pointer memory_partition_pos) ;
//void* get_pointer_to_block_from_file(t_Block_Pointer file_block_pos);
void block_msync(void* get_pointer_to_memory) ;
void write_block(void* pointer_to_block, void* ptro_datos, size_t desplazamiento) ;
void create_metadata_file(const char *filename, size_t size, t_Block_Pointer index_block) ;

void write_complete_index();
void write_Complete_data () ;

bool is_address_in_mapped_area(void *addr) ;	
void print_memory_as_ints(void *ptro_memory_dump_block) ;
bool is_address_in_mapped_area(void *addr) ;
int create_directory(const char *path);
void print_size_t_array(void *array, size_t total_size) ;

void *get_pointer_to_memory_dump(void * memory_ptr, size_t memory_partition_size, t_Block_Pointer memory_partition_pos) ;
void set_bitmap_bits_free(t_Bitmap * bit_map);

#endif // FILESYSTEM_H