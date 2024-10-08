/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/send.h"

// Handshake

int send_port_type(e_Port_Type port_type, int fd_socket) {
  t_Package *package = package_create_with_header(PORT_TYPE_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(port_type_serialize(&(package->payload), port_type)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_port_type(e_Port_Type *port_type, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != PORT_TYPE_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(port_type_deserialize(&(package->payload), port_type)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

// De uso general

int send_header(e_Header header, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    goto error;
  }

  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_expected_header(e_Header expected_header, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

int send_data_with_header(e_Header header, void *data, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    goto error;
  }

  if(data_serialize(&(package->payload), data, bytes)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_data_with_expected_header(e_Header expected_header, void **data, size_t *bytes, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(data_deserialize(&(package->payload), data, bytes)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

int send_text_with_header(e_Header header, char *text, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    goto error;
  }

  if(text_serialize(&(package->payload), text)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_text_with_expected_header(e_Header expected_header, char **text, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(text_deserialize(&(package->payload), text)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

int send_result_with_header(e_Header header, int result, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    goto error;
  }

  if(result_serialize(&(package->payload), result)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_result_with_expected_header(e_Header expected_header, int *result, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(result_deserialize(&(package->payload), result)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

int send_pid_and_tid_with_header(e_Header header, t_PID pid, t_TID tid, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_pid_and_tid_with_expected_header(e_Header expected_header, t_PID *pid, t_TID *tid, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(payload_remove(&(package->payload), pid, sizeof(*pid))) {
    goto error_package;
  }
  if(payload_remove(&(package->payload), tid, sizeof(*tid))) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

// Kernel - Memoria

int send_process_create(t_PID pid, size_t size, int fd_socket) {
  t_Package *package = package_create_with_header(PROCESS_CREATE_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(size_serialize(&(package->payload), size)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int send_process_destroy(t_PID pid, int fd_socket) {
  t_Package *package = package_create_with_header(PROCESS_DESTROY_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int send_thread_create(t_PID pid, t_TID tid, char *instructions_path, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_CREATE_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(text_serialize(&(package->payload), instructions_path)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int send_thread_destroy(t_PID pid, t_TID tid, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_DESTROY_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

// Kernel - CPU

int send_thread_eviction(e_Eviction_Reason eviction_reason, t_Payload syscall_instruction, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_EVICTION_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(eviction_reason_serialize(&(package->payload), eviction_reason)) {
    goto error_package;
  }
  if(subpayload_serialize(&(package->payload), syscall_instruction)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_thread_eviction(e_Eviction_Reason *eviction_reason, t_Payload *syscall_instruction, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != THREAD_EVICTION_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(eviction_reason_deserialize(&(package->payload), eviction_reason)) {
    goto error_package;
  }
  if(subpayload_deserialize(&(package->payload), syscall_instruction)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

int send_kernel_interrupt(e_Kernel_Interrupt type, t_PID pid, t_TID tid, int fd_socket) {
	t_Package *package = package_create_with_header(KERNEL_INTERRUPT_HEADER);
  if(package == NULL) {
    goto error;
  }

	if(kernel_interrupt_serialize(&(package->payload), type)) {
    goto error_package;
  }
	if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
	if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

	package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_kernel_interrupt(e_Kernel_Interrupt *kernel_interrupt, t_PID *pid, t_TID *tid, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != KERNEL_INTERRUPT_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(kernel_interrupt_deserialize(&(package->payload), kernel_interrupt)) {
    goto error_package;
  }
  if(payload_remove(&(package->payload), pid, sizeof(*pid))) {
    goto error_package;
  }
  if(payload_remove(&(package->payload), tid, sizeof(*tid))) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

// CPU - Memoria

int send_exec_context(t_Exec_Context exec_context, int fd_socket) {
  t_Package *package = package_create_with_header(EXEC_CONTEXT_REQUEST_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(exec_context_serialize(&(package->payload), exec_context)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int receive_exec_context(t_Exec_Context *exec_context, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != EXEC_CONTEXT_REQUEST_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(exec_context_deserialize(&(package->payload), exec_context)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}

int send_instruction_request(t_PID pid, t_TID tid, t_PC pc, int fd_socket) {
  t_Package *package = package_create_with_header(INSTRUCTION_REQUEST_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &pc, sizeof(pc))) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int send_write_request(t_PID pid, t_TID tid, size_t physical_address, void *data, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(WRITE_REQUEST_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(size_serialize(&(package->payload), physical_address)) {
    goto error_package;
  }
  if(data_serialize(&(package->payload), data, bytes)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int send_read_request(t_PID pid, t_TID tid, size_t physical_address, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(READ_REQUEST_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(size_serialize(&(package->payload), physical_address)) {
    goto error_package;
  }
  if(size_serialize(&(package->payload), bytes)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

int send_exec_context_update(t_PID pid, t_TID tid, t_Exec_Context exec_context, int fd_socket) {
  t_Package *package = package_create_with_header(EXEC_CONTEXT_UPDATE_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    goto error_package;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    goto error_package;
  }
  if(exec_context_serialize(&(package->payload), exec_context)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}

// Memoria - Filesystem

int send_memory_dump(char *filename, void *dump, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(MEMORY_DUMP_HEADER);
  if(package == NULL) {
    goto error;
  }

  if(text_serialize(&(package->payload), filename)) {
    goto error_package;
  }
  if(data_serialize(&(package->payload), dump, bytes)) {
    goto error_package;
  }
  if(package_send(package, fd_socket)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
  error:
    return -1;
}


int receive_memory_dump(char **filename, void **dump, size_t *bytes, int fd_socket) {
  t_Package *package;

  if(package_receive(&package, fd_socket)) {
    goto error_package;
  }
  if(package->header != MEMORY_DUMP_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    goto error_package;
  }
  if(text_deserialize(&(package->payload), filename)) {
    goto error_package;
  }
  if(data_deserialize(&(package->payload), *dump, bytes)) {
    goto error_package;
  }

  package_destroy(package);
  return 0;

  error_package:
    package_destroy(package);
    return -1;
}