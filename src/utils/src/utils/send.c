/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/send.h"

// Handshake

int send_port_type(e_Port_Type port_type, int fd_socket) {
  t_Package *package = package_create_with_header(PORT_TYPE_HEADER);
  if(port_type_serialize(&(package->payload), port_type)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int receive_port_type(e_Port_Type *port_type, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket)) {
    return -1;
  }
  if(package->header != PORT_TYPE_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  if(port_type_deserialize(&(package->payload), port_type)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

// De uso general

int send_header(e_Header header, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int receive_expected_header(e_Header expected_header, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_text_with_header(e_Header header, char *text, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(text_serialize(&(package->payload), text)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int receive_text_with_expected_header(e_Header expected_header, char **text, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
  return -1;
  }
  if(text_deserialize(&(package->payload), text)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_return_value_with_header(e_Header header, t_Return_Value return_value, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(return_value_serialize(&(package->payload), return_value)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int receive_return_value_with_expected_header(e_Header expected_header, t_Return_Value *return_value, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  if(return_value_deserialize(&(package->payload), return_value)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_pid_and_tid_with_header(e_Header header, t_PID pid, t_TID tid, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int receive_pid_and_tid_with_expected_header(e_Header expected_header, t_PID *pid, t_TID *tid, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  if(payload_remove(&(package->payload), pid, sizeof(*pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_remove(&(package->payload), tid, sizeof(*tid))) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

// Kernel - Memoria

int send_process_create(t_PID pid, size_t size, int fd_socket) {
  t_Package *package = package_create_with_header(PROCESS_CREATE_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(size_serialize(&(package->payload), size)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_process_destroy(t_PID pid, int fd_socket) {
  t_Package *package = package_create_with_header(PROCESS_DESTROY_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_thread_create(t_PID pid, t_TID tid, char *instructions_path, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_CREATE_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(text_serialize(&(package->payload), instructions_path)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_thread_destroy(t_PID pid, t_TID tid, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_DESTROY_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

// Kernel - CPU

int send_thread_eviction(e_Eviction_Reason eviction_reason, t_Payload syscall_instruction, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_EVICTION_HEADER);
  if(eviction_reason_serialize(&(package->payload), eviction_reason)) {
    package_destroy(package);
    return -1;
  }
  if(subpayload_serialize(&(package->payload), syscall_instruction)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int receive_thread_eviction(e_Eviction_Reason *eviction_reason, t_Payload *syscall_instruction, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != THREAD_EVICTION_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  if(eviction_reason_deserialize(&(package->payload), eviction_reason)) {
    package_destroy(package);
    return -1;
  }
  if(subpayload_deserialize(&(package->payload), syscall_instruction)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_kernel_interrupt(e_Kernel_Interrupt type, t_PID pid, t_TID tid, int fd_socket) {
	t_Package *package = package_create_with_header(KERNEL_INTERRUPT_HEADER);
	if(kernel_interrupt_serialize(&(package->payload), type)) {
    package_destroy(package);
    return -1;
  }
	if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
	if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
	package_destroy(package);
  return 0;
}

int receive_kernel_interrupt(e_Kernel_Interrupt *kernel_interrupt, t_PID *pid, t_TID *tid, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != KERNEL_INTERRUPT_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  if(kernel_interrupt_deserialize(&(package->payload), kernel_interrupt)) {
    package_destroy(package);
    return -1;
  }
  if(payload_remove(&(package->payload), pid, sizeof(*pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_remove(&(package->payload), tid, sizeof(*tid))) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

// CPU - Memoria

int send_exec_context(t_Exec_Context exec_context, int fd_socket) {
  t_Package *package = package_create_with_header(EXEC_CONTEXT_REQUEST_HEADER);
  if(exec_context_serialize(&(package->payload), exec_context)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int receive_exec_context(t_Exec_Context *exec_context, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != EXEC_CONTEXT_REQUEST_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  if(exec_context_deserialize(&(package->payload), exec_context)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_instruction_request(t_PID pid, t_TID tid, t_PC pc, int fd_socket) {
  t_Package *package = package_create_with_header(INSTRUCTION_REQUEST_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &pc, sizeof(pc))) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_instruction(t_PID pid, t_TID tid, char* instruction, int fd_socket) {
  t_Package *package = package_create_with_header(INSTRUCTION_REQUEST_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(text_serialize(&(package->payload), instruction)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_write_request(t_PID pid, t_TID tid, size_t physical_address, void *data, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(WRITE_REQUEST_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(size_serialize(&(package->payload), physical_address)) {
    package_destroy(package);
    return -1;
  }
  if(data_serialize(&(package->payload), data, bytes)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_read_request(t_PID pid, t_TID tid, size_t physical_address, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(READ_REQUEST_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(size_serialize(&(package->payload), physical_address)) {
    package_destroy(package);
    return -1;
  }
  if(size_serialize(&(package->payload), bytes)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

int send_exec_context_update(t_PID pid, t_TID tid, t_Exec_Context exec_context, int fd_socket) {
  t_Package *package = package_create_with_header(EXEC_CONTEXT_UPDATE_HEADER);
  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    package_destroy(package);
    return -1;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    package_destroy(package);
    return -1;
  }
  if(exec_context_serialize(&(package->payload), exec_context)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}

// Memoria - Filesystem

int send_memory_dump(char *filename, void *dump, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(MEMORY_DUMP_HEADER);
  if(text_serialize(&(package->payload), filename)) {
    package_destroy(package);
    return -1;
  }
  if(data_serialize(&(package->payload), dump, bytes)) {
    package_destroy(package);
    return -1;
  }
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}


int receive_memory_dump(char **filename, void **dump, size_t *bytes, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return -1;
  if(package->header != MEMORY_DUMP_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return -1;
  }
  if(text_deserialize(&(package->payload), filename)) {
    package_destroy(package);
    return -1;
  }
  if(data_deserialize(&(package->payload), *dump, bytes)) {
    package_destroy(package);
    return -1;
  }
  package_destroy(package);
  return 0;
}