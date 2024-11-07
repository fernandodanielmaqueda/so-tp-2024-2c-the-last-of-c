/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "package.h"

const char *HEADER_NAMES[] = {
    // Handshake
    [PORT_TYPE_HEADER] = "PORT_TYPE_HEADER",
    // Kernel <==> CPU
    [THREAD_DISPATCH_HEADER] = "THREAD_DISPATCH_HEADER",
    [THREAD_EVICTION_HEADER] = "PROCESS_EVICTION_HEADER",
    [KERNEL_INTERRUPT_HEADER] = "KERNEL_INTERRUPT_HEADER",
    // Kernel <==> Memoria
    [PROCESS_CREATE_HEADER] = "PROCESS_CREATE_HEADER",
    [PROCESS_DESTROY_HEADER] = "PROCESS_DESTROY_HEADER",
    [THREAD_CREATE_HEADER] = "THREAD_CREATE_HEADER",
    [THREAD_DESTROY_HEADER] = "THREAD_DESTROY_HEADER",
    // CPU <==> Memoria
    [EXEC_CONTEXT_REQUEST_HEADER] = "EXEC_CONTEXT_REQUEST_HEADER",
    [EXEC_CONTEXT_UPDATE_HEADER] = "EXEC_CONTEXT_UPDATE_HEADER",
    [INSTRUCTION_REQUEST_HEADER] = "INSTRUCTION_REQUEST",
    [READ_REQUEST_HEADER] = "READ_REQUEST",
    [WRITE_REQUEST_HEADER] = "WRITE_REQUEST",
    //Memoria <==> Filesystem
    [MEMORY_DUMP_HEADER] = "MEMORY_DUMP_HEADER"
};

t_Package *package_create(void) {

  t_Package *package = malloc(sizeof(t_Package));
  if(package == NULL) {
    log_error(SERIALIZE_LOGGER, "malloc: No se pudieron reservar %zu bytes para crear el paquete", sizeof(t_Package));
    return NULL;
  }

  payload_init(&(package->payload));

  return package;
}

t_Package *package_create_with_header(e_Header header) {
  t_Package *package = package_create();
  if(package == NULL) {
    return NULL;
  }

  package->header = header;

  return package;
}

void package_destroy(t_Package *package) {
  if(package == NULL)
    return;
  payload_destroy(&(package->payload));
  free(package);
}

int package_send(t_Package *package, int fd_socket) {
  
  // Si el paquete es NULL, no se envia nada
  if(package == NULL)
    return -1;

  size_t previous_offset = package->payload.offset;

  payload_seek(&(package->payload), 0, SEEK_SET);

  t_EnumValue header_serialized;
  header_serialized = (t_EnumValue) package->header;

  t_Size size_serialized;
  size_serialized = (t_Size) package->payload.size;

  payload_add(&(package->payload), &(header_serialized), sizeof(header_serialized));
  payload_add(&(package->payload), &(size_serialized), sizeof(size_serialized));

  size_t bufferSize = package->payload.size;

  ssize_t bytes = send(fd_socket, package->payload.stream, package->payload.size, 0);

  payload_seek(&(package->payload), 0, SEEK_SET);

  payload_remove(&(package->payload), NULL, sizeof(header_serialized));
  payload_remove(&(package->payload), NULL, sizeof(size_serialized));

  payload_seek(&(package->payload), previous_offset, SEEK_CUR);

  if(bytes == -1) {
      log_warning(SERIALIZE_LOGGER, "send: %s\n", strerror(errno));
      return -1;
  }
  if(bytes != bufferSize) {
      log_warning(SERIALIZE_LOGGER, "send: No coinciden los bytes enviados (%zd) con los que se esperaban enviar (%zd)\n", bufferSize, bytes);
      return -1;
  }

  return 0;
}

int package_receive(t_Package *package, int fd_socket) {
  if(package == NULL)
    return -1;

  if(package_receive_header(package, fd_socket))
    return -1;

  if(package_receive_payload(package, fd_socket))
    return -1;

  return 0;
}

int package_receive_header(t_Package *package, int fd_socket) {

  if(package == NULL)
    return -1;

  t_EnumValue aux;

  if(receive(fd_socket, (void *) &(aux), sizeof(aux)))
    return -1;

  package->header = (e_Header) aux;

  return 0;
}

int package_receive_payload(t_Package *package, int fd_socket) {

  if(package == NULL)
    return -1;

  t_Size aux;

  if(receive(fd_socket, (void *) &(aux), sizeof(aux)))
    return -1;

  package->payload.size = (size_t) aux;

  if(package->payload.size == 0)
    return 0;

  package->payload.stream = malloc((size_t) package->payload.size);
  if(package->payload.stream == NULL) {
    log_error(SERIALIZE_LOGGER, "malloc: No se pudieron reservar %zu bytes para recibir el stream del payload", (size_t) package->payload.size);
    return -1;
  }

  return receive(fd_socket, (void *) package->payload.stream, (size_t) package->payload.size);
}

int receive(int fd_socket, void *destination, size_t expected_bytes) {

  ssize_t bytes = recv(fd_socket, destination, expected_bytes, 0); // MSG_WAITALL
  if(bytes == 0) {
      log_warning(SERIALIZE_LOGGER, "[%d] recv: No hay mensajes disponibles para recibir y el par ha realizado un cierre ordenado\n", fd_socket);
      return -1;
  }
  if(bytes == -1) {
      log_warning(SERIALIZE_LOGGER, "[%d] recv: %s\n", fd_socket, strerror(errno));
      return -1;
  }
  if(bytes != expected_bytes) {
      log_warning(SERIALIZE_LOGGER, "[%d] recv: No coinciden los bytes recibidos (%zu) con los que se esperaban recibir (%zd)\n", fd_socket, expected_bytes, bytes);
      return -1;
  }

  return 0;
}