/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_SEND_H
#define UTILS_SEND_H

#include "utils/package.h"
#include "utils/serialize/cpu_opcode.h"
#include "utils/serialize/eviction_reason.h"
#include "utils/serialize/exec_context.h"
#include "utils/serialize/kernel_interrupt.h"
#include "utils/serialize/list.h"
#include "utils/serialize/port_type.h"
#include "utils/serialize/return_value.h"
#include "utils/serialize/size.h"
#include "utils/serialize/subheader.h"
#include "utils/serialize/subpayload.h"
#include "utils/serialize/text.h"

// Handshake


int send_port_type(e_Port_Type port_type, int fd_socket);


int receive_port_type(e_Port_Type *port_type, int fd_socket);


// De uso general


int send_header(e_Header header, int fd_socket);


int receive_expected_header(e_Header header, int fd_socket);


int send_text_with_header(e_Header header, char *text, int fd_socket);


int receive_text_with_expected_header(e_Header header, char **text, int fd_socket);


int send_return_value_with_header(e_Header header, t_Return_Value return_value, int fd_socket);


int receive_return_value_with_expected_header(e_Header expected_header, t_Return_Value *return_value, int fd_socket);


int send_pid_and_tid_with_header(e_Header header, t_PID pid, t_TID tid, int fd_socket);


int receive_pid_and_tid_with_expected_header(e_Header expected_header, t_PID *pid, t_TID *tid, int fd_socket);

// Kernel - Memoria


int send_process_create(t_PID pid, int fd_socket);


int send_process_destroy(t_PID pid, int fd_socket);


int send_thread_create(t_PID pid, t_TID tid, char *instructions_path, int fd_socket);


int send_thread_destroy(t_PID pid, t_TID tid, int fd_socket);


// Kernel - CPU

int send_thread_eviction(e_Eviction_Reason eviction_reason, t_Payload syscall_instruction, int fd_socket);


int receive_thread_eviction(e_Eviction_Reason *eviction_reason, t_Payload *syscall_instruction, int fd_socket);


int send_kernel_interrupt(e_Kernel_Interrupt type, t_PID pid, t_TID tid, int fd_socket);


int receive_kernel_interrupt(e_Kernel_Interrupt *kernel_interrupt, t_PID *pid, t_TID *tid, int fd_socket);


// CPU - Memoria

int send_exec_context(t_Exec_Context exec_context, size_t base, size_t limit, int fd_socket);


int receive_exec_context(t_Exec_Context *exec_context, size_t *base, size_t *limit, int fd_socket);


int send_instruction_request(t_PID pid, t_TID tid, t_PC pc, int fd_socket);


int send_write_request(t_PID pid, t_TID tid, size_t physical_address, void *source, size_t bytes, int fd_socket);


int send_read_request(t_PID pid, t_TID tid, size_t physical_address, size_t bytes, int fd_socket);


int send_exec_context_update(t_PID pid, t_TID tid, t_Exec_Context exec_context, int fd_socket);


// Memoria - Filesystem

int send_memory_dump(char *filename, void *dump, size_t bytes, int fd_socket);


int receive_memory_dump(char **filename, void **dump, size_t *bytes, int fd_socket);

#endif // UTILS_SEND_H