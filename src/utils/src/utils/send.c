/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "utils/send.h"

// Handshake

int send_port_type(e_Port_Type port_type, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(PORT_TYPE_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(port_type_serialize(&(package->payload), port_type)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_port_type(e_Port_Type *port_type, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != PORT_TYPE_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(port_type_deserialize(&(package->payload), port_type)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

// De uso general

int send_header(e_Header header, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_expected_header(e_Header expected_header, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_data_with_header(e_Header header, void *data, size_t bytes, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(data_serialize(&(package->payload), data, bytes)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_data_with_expected_header(e_Header expected_header, void **data, size_t *bytes, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(data_deserialize(&(package->payload), data, bytes)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_text_with_header(e_Header header, char *text, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(text_serialize(&(package->payload), text)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_text_with_expected_header(e_Header expected_header, char **text, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(text_deserialize(&(package->payload), text)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_result_with_header(e_Header header, int result, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(result_serialize(&(package->payload), result)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_result_with_expected_header(e_Header expected_header, int *result, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(result_deserialize(&(package->payload), result)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_pid_and_tid_with_header(e_Header header, t_PID pid, t_TID tid, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(header);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_pid_and_tid_with_expected_header(e_Header expected_header, t_PID *pid, t_TID *tid, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != expected_header) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(payload_remove(&(package->payload), pid, sizeof(*pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_remove(&(package->payload), tid, sizeof(*tid))) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

// Kernel - Memoria

int send_process_create(t_PID pid, size_t size, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(PROCESS_CREATE_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(size_serialize(&(package->payload), size)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_process_destroy(t_PID pid, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(PROCESS_DESTROY_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_thread_create(t_PID pid, t_TID tid, char *instructions_path, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(THREAD_CREATE_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(text_serialize(&(package->payload), instructions_path)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_thread_destroy(t_PID pid, t_TID tid, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(THREAD_DESTROY_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

// Kernel - CPU

int send_thread_eviction(e_Eviction_Reason eviction_reason, t_Payload syscall_instruction, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(THREAD_EVICTION_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(eviction_reason_serialize(&(package->payload), eviction_reason)) {
    retval = -1;
    goto cleanup;
  }
  if(subpayload_serialize(&(package->payload), syscall_instruction)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_thread_eviction(e_Eviction_Reason *eviction_reason, t_Payload *syscall_instruction, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != THREAD_EVICTION_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(eviction_reason_deserialize(&(package->payload), eviction_reason)) {
    retval = -1;
    goto cleanup;
  }
  if(subpayload_deserialize(&(package->payload), syscall_instruction)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_kernel_interrupt(e_Kernel_Interrupt type, t_PID pid, t_TID tid, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(KERNEL_INTERRUPT_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

	if(kernel_interrupt_serialize(&(package->payload), type)) {
    retval = -1;
    goto cleanup;
  }
	if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
	if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_kernel_interrupt(e_Kernel_Interrupt *kernel_interrupt, t_PID *pid, t_TID *tid, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != KERNEL_INTERRUPT_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(kernel_interrupt_deserialize(&(package->payload), kernel_interrupt)) {
    retval = -1;
    goto cleanup;
  }
  if(payload_remove(&(package->payload), pid, sizeof(*pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_remove(&(package->payload), tid, sizeof(*tid))) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

// CPU - Memoria

int send_exec_context(t_Exec_Context exec_context, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(EXEC_CONTEXT_REQUEST_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(exec_context_serialize(&(package->payload), exec_context)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int receive_exec_context(t_Exec_Context *exec_context, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != EXEC_CONTEXT_REQUEST_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(exec_context_deserialize(&(package->payload), exec_context)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_instruction_request(t_PID pid, t_TID tid, t_PC pc, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(INSTRUCTION_REQUEST_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &pc, sizeof(pc))) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_write_request(t_PID pid, t_TID tid, size_t physical_address, void *data, size_t bytes, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(WRITE_REQUEST_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(size_serialize(&(package->payload), physical_address)) {
    retval = -1;
    goto cleanup;
  }
  if(data_serialize(&(package->payload), data, bytes)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_read_request(t_PID pid, t_TID tid, size_t physical_address, size_t bytes, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(READ_REQUEST_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(size_serialize(&(package->payload), physical_address)) {
    retval = -1;
    goto cleanup;
  }
  if(size_serialize(&(package->payload), bytes)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

int send_exec_context_update(t_PID pid, t_TID tid, t_Exec_Context exec_context, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(EXEC_CONTEXT_UPDATE_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(payload_add(&(package->payload), &pid, sizeof(pid))) {
    retval = -1;
    goto cleanup;
  }
  if(payload_add(&(package->payload), &tid, sizeof(tid))) {
    retval = -1;
    goto cleanup;
  }
  if(exec_context_serialize(&(package->payload), exec_context)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}

// Memoria - Filesystem

int send_memory_dump(char *filename, void *dump, size_t bytes, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create_with_header(MEMORY_DUMP_HEADER);
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(text_serialize(&(package->payload), filename)) {
    retval = -1;
    goto cleanup;
  }
  if(data_serialize(&(package->payload), dump, bytes)) {
    retval = -1;
    goto cleanup;
  }
  if(package_send(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}


int receive_memory_dump(char **filename, void **dump, size_t *bytes, int fd_socket) {
  int retval = 0;

  t_Package *package = package_create();
  if(package == NULL) {
    return -1;
  }
  pthread_cleanup_push((void (*)(void *)) package_destroy, package);

  if(package_receive(package, fd_socket)) {
    retval = -1;
    goto cleanup;
  }
  if(package->header != MEMORY_DUMP_HEADER) {
    log_error(SERIALIZE_LOGGER, "%s: Header invalido [%d]", HEADER_NAMES[package->header], package->header);
    retval = -1;
    goto cleanup;
  }
  if(text_deserialize(&(package->payload), filename)) {
    retval = -1;
    goto cleanup;
  }
  if(data_deserialize(&(package->payload), dump, bytes)) {
    retval = -1;
    goto cleanup;
  }

  cleanup:
  pthread_cleanup_pop(1);
  return retval;
}