/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_PACKAGE_H
#define UTILS_PACKAGE_H

#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include "commons/config.h"
#include "commons/log.h"
#include "utils/payload.h"
#include "utils/module.h"

typedef int8_t t_EnumValue;

typedef enum e_Header {
    // Handshake
    PORT_TYPE_HEADER,
    // Kernel <==> CPU
    THREAD_DISPATCH_HEADER,
    THREAD_EVICTION_HEADER,
    KERNEL_INTERRUPT_HEADER,
    // Kernel <==> Memoria
    PROCESS_CREATE_HEADER,
    PROCESS_DESTROY_HEADER,
    THREAD_CREATE_HEADER,
    THREAD_DESTROY_HEADER,
    //Kernel <==> Filesystem
    INTERFACE_DATA_REQUEST_HEADER,
    IO_OPERATION_DISPATCH_HEADER,
    IO_OPERATION_FINISHED_HEADER,
    // CPU <==> Memoria
    EXEC_CONTEXT_REQUEST_HEADER,
    EXEC_CONTEXT_UPDATE_HEADER,
    INSTRUCTION_REQUEST_HEADER,
    READ_REQUEST_HEADER,
    WRITE_REQUEST_HEADER,
    COPY_REQUEST_HEADER,
    RESIZE_REQUEST_HEADER,
    FRAME_ACCESS_HEADER,
    FRAME_REQUEST_HEADER,
    PAGE_SIZE_REQUEST_HEADER,
    //Filesystem <==> Memoria
    IO_FS_WRITE_MEMORY_HEADER,
    IO_FS_READ_MEMORY_HEADER,
    IO_STDIN_WRITE_MEMORY_HEADER,
    IO_STDOUT_READ_MEMORY_HEADER
} e_Header;

typedef struct t_Package {
    enum e_Header header;
    t_Payload payload;
} t_Package;

extern const char *HEADER_NAMES[];

/**
 * @brief Crear paquete.
 */
t_Package *package_create(void);

/**
 * @brief Crear paquete.
 * @param header Codigo de operacion que tendra el paquete.
 */
t_Package *package_create_with_header(e_Header header);

/**
 * @brief Eliminar paquete
 * @param package t_Package a eliminar.
 */
void package_destroy(t_Package *package);

/**
 * @brief Obtiene el codigo de operacion de un paquete
 * @param package Paquete a enviar
 * @param fd_socket Socket destino
 */
int package_send(t_Package *package, int fd_socket);

int package_receive(t_Package **destination, int fd_socket);

/**
 * @brief Obtiene el codigo de operacion de un paquete
 * @param package Paquete
 * @param fd_socket Paquete donde se creara el payload
 */
int package_receive_header(t_Package *package, int fd_socket);

int package_receive_payload(t_Package *package, int fd_socket);

int receive(int fd_socket, void *destination, size_t expected_bytes);

#endif // UTILS_PACKAGE_H