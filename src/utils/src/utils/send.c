/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/send.h"

// Handshake

int send_port_type(e_Port_Type port_type, int fd_socket) {
  t_Package *package = package_create_with_header(PORT_TYPE_HEADER);
  port_type_serialize(&(package->payload), port_type);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int receive_port_type(e_Port_Type *port_type, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == PORT_TYPE_HEADER)
    port_type_deserialize(&(package->payload), port_type);
  else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

// De uso general

int send_header(e_Header header, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int receive_expected_header(e_Header expected_header, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_text_with_header(e_Header header, char *text, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  text_serialize(&(package->payload), text);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int receive_text_with_expected_header(e_Header expected_header, char **text, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == expected_header)
      text_deserialize(&(package->payload), text);
  else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_return_value_with_header(e_Header header, t_Return_Value return_value, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  return_value_serialize(&(package->payload), return_value);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int receive_return_value_with_expected_header(e_Header expected_header, t_Return_Value *return_value, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == expected_header)
    return_value_deserialize(&(package->payload), return_value);
  else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_pid_and_tid_with_header(e_Header header, t_PID pid, t_TID tid, int fd_socket) {
  t_Package *package = package_create_with_header(header);
  payload_add(&(package->payload), &pid, sizeof(pid));
  payload_add(&(package->payload), &tid, sizeof(tid));
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int receive_pid_and_tid_with_expected_header(e_Header expected_header, t_PID *pid, t_TID *tid, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == expected_header) {
    payload_remove(&(package->payload), pid, sizeof(*pid));
    payload_remove(&(package->payload), tid, sizeof(*tid));
  } else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

// Kernel - Memoria

int send_process_create(t_PID pid, int fd_socket) {
  t_Package *package = package_create_with_header(PROCESS_CREATE_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_process_destroy(t_PID pid, int fd_socket) {
  t_Package *package = package_create_with_header(PROCESS_DESTROY_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_thread_create(t_PID pid, t_TID tid, char *instructions_path, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_CREATE_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  payload_add(&(package->payload), &tid, sizeof(tid));
  text_serialize(&(package->payload), instructions_path);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_thread_destroy(t_PID pid, t_TID tid, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_DESTROY_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  payload_add(&(package->payload), &tid, sizeof(tid));
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

// Kernel - CPU

int send_thread_eviction(e_Eviction_Reason eviction_reason, t_Payload syscall_instruction, int fd_socket) {
  t_Package *package = package_create_with_header(THREAD_EVICTION_HEADER);
  eviction_reason_serialize(&(package->payload), eviction_reason);
  subpayload_serialize(&(package->payload), syscall_instruction);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int receive_thread_eviction(e_Eviction_Reason *eviction_reason, t_Payload *syscall_instruction, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == THREAD_EVICTION_HEADER) {
    eviction_reason_deserialize(&(package->payload), eviction_reason);
    subpayload_deserialize(&(package->payload), syscall_instruction);
  } else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_kernel_interrupt(e_Kernel_Interrupt type, t_PID pid, t_TID tid, int fd_socket) {
	t_Package *package = package_create_with_header(KERNEL_INTERRUPT_HEADER);
	kernel_interrupt_serialize(&(package->payload), type);
	payload_add(&(package->payload), &pid, sizeof(pid));
	payload_add(&(package->payload), &tid, sizeof(tid));
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
	package_destroy(package);
  return 0;
}

int receive_kernel_interrupt(e_Kernel_Interrupt *kernel_interrupt, t_PID *pid, t_TID *tid, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == KERNEL_INTERRUPT_HEADER) {
    kernel_interrupt_deserialize(&(package->payload), kernel_interrupt);
    payload_remove(&(package->payload), pid, sizeof(*pid));
    payload_remove(&(package->payload), tid, sizeof(*tid));
  } else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

// CPU - Memoria

int send_exec_context(t_Exec_Context exec_context, size_t base, size_t limit, int fd_socket) {
  t_Package *package = package_create_with_header(EXEC_CONTEXT_REQUEST_HEADER);
  exec_context_serialize(&(package->payload), exec_context);
  size_serialize(&(package->payload), base);
  size_serialize(&(package->payload), limit);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int receive_exec_context(t_Exec_Context *exec_context, size_t *base, size_t *limit, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == EXEC_CONTEXT_REQUEST_HEADER) {
    exec_context_deserialize(&(package->payload), exec_context);
    size_deserialize(&(package->payload), base);
    size_deserialize(&(package->payload), limit);
  } else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_instruction_request(t_PID pid, t_TID tid, t_PC pc, int fd_socket) {
  t_Package *package = package_create_with_header(INSTRUCTION_REQUEST_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  payload_add(&(package->payload), &tid, sizeof(tid));
  payload_add(&(package->payload), &pc, sizeof(pc));
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_write_request(t_PID pid, t_TID tid, size_t physical_address, void *source, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(WRITE_REQUEST_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  payload_add(&(package->payload), &tid, sizeof(tid));
  size_serialize(&(package->payload), physical_address);
  size_serialize(&(package->payload), bytes);
  payload_add(&(package->payload), source, bytes);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_read_request(t_PID pid, t_TID tid, size_t physical_address, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(READ_REQUEST_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  payload_add(&(package->payload), &tid, sizeof(tid));
  size_serialize(&(package->payload), physical_address);
  size_serialize(&(package->payload), bytes);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

int send_exec_context_update(t_PID pid, t_TID tid, t_Exec_Context exec_context, int fd_socket) {
  t_Package *package = package_create_with_header(EXEC_CONTEXT_UPDATE_HEADER);
  payload_add(&(package->payload), &pid, sizeof(pid));
  payload_add(&(package->payload), &tid, sizeof(tid));
  exec_context_serialize(&(package->payload), exec_context);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}

// Memoria - Filesystem

int send_memory_dump(char *filename, void *dump, size_t bytes, int fd_socket) {
  t_Package *package = package_create_with_header(MEMORY_DUMP_HEADER);
  text_serialize(&(package->payload), filename);
  size_serialize(&(package->payload), bytes);
  payload_add(&(package->payload), dump, bytes);
  if(package_send(package, fd_socket)) {
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}


int receive_memory_dump(char **filename, void **destination, size_t *bytes, int fd_socket) {
  t_Package *package;
  if(package_receive(&package, fd_socket))
    return 1;
  if(package->header == MEMORY_DUMP_HEADER) {
    text_deserialize(&(package->payload), filename);
    size_deserialize(&(package->payload), bytes);
    payload_remove(&(package->payload), destination, *bytes);
  } else {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido (%d)", HEADER_NAMES[package->header], package->header);
    package_destroy(package);
    return 1;
  }
  package_destroy(package);
  return 0;
}