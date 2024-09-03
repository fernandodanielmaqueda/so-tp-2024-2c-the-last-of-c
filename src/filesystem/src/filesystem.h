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


typedef struct t_IO_Type {
    char *name;
    void (*function) (void);
} t_IO_Type;

typedef struct t_IO_Operation {
    char *name;
    int (*function) (t_Payload *);
} t_IO_Operation;

typedef struct t_FS_File {
    char *name;
//    t_PID process_pid;
    uint32_t initial_bloq;
    uint32_t len;
    uint32_t size;
} t_FS_File;

extern char *INTERFACE_NAME;

extern int WORK_UNIT_TIME;

extern t_Connection CONNECTION_KERNEL;
extern t_Connection CONNECTION_MEMORY;

extern char *PATH_BASE_DIALFS;
extern size_t BLOCK_SIZE;
extern size_t BLOCK_COUNT;
extern int COMPRESSION_DELAY;

//extern t_IO_Type IO_TYPES[];

//extern e_IO_Type IO_TYPE;

//extern t_IO_Operation IO_OPERATIONS[];

int module(int, char*[]);
void read_module_config(t_config *module_config);
//int io_type_find(char *name, e_IO_Type *destination);
void initialize_sockets(void);
void finish_sockets(void);
void generic_interface_function(void);
void stdin_interface_function(void);
void stdout_interface_function(void);
void dialfs_interface_function(void);
uint32_t seek_first_free_block();
t_FS_File* seek_file(char* file_name);
bool can_assign_block(size_t initial_position, size_t len, size_t final_len);
size_t seek_quantity_blocks_required(size_t puntero, size_t bytes);
void initialize_bitmap();
void initialize_blocks();
void free_bitmap_blocks();
void create_file(char* file_name, size_t first_block);
void update_file(char* file_name, size_t size, size_t location);
int quantity_free_blocks();
void compact_blocks(t_FS_File* file, size_t qBlocks, size_t newSize);
t_FS_File* seek_file_by_header_index(size_t position);
void moveBlock(size_t blocks_to_move, size_t free_spaces, size_t location);

#endif // FILESYSTEM_H