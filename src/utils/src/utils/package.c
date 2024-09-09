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
    log_error(SOCKET_LOGGER, "No se pudo crear el package con malloc");
    exit(EXIT_FAILURE);
  }

  payload_init(&(package->payload));

  return package;
}

t_Package *package_create_with_header(e_Header header) {
  t_Package *package = package_create();
  package->header = header;
  return package;
}

void package_destroy(t_Package *package) {
  if (package == NULL)
    return;
  payload_destroy(&(package->payload));
  free(package);
}

int package_send(t_Package *package, int fd_socket) {
  
  // Si el paquete es NULL, no se envia nada
  if(package == NULL)
    return 1;

  size_t previous_offset = package->payload.offset;
  payload_seek(&(package->payload), 0, SEEK_SET);

  t_EnumValue aux_header;
  aux_header = (t_EnumValue) package->header;
  payload_add(&(package->payload), &(aux_header), sizeof(aux_header));

  t_Size aux_size;
  aux_size = (t_Size) package->payload.size;
  payload_add(&(package->payload), &(aux_size), sizeof(aux_size));

  size_t bufferSize = package->payload.size;

  ssize_t bytes = send(fd_socket, package->payload.stream, package->payload.size, 0);

  payload_seek(&(package->payload), 0, SEEK_SET);

  payload_remove(&(package->payload), NULL, sizeof(aux_header));
  payload_remove(&(package->payload), NULL, sizeof(aux_size));

  payload_seek(&(package->payload), previous_offset, SEEK_CUR);

  if (bytes == -1) {
      log_warning(SOCKET_LOGGER, "send: %s\n", strerror(errno));
      return 1;
  }
  if (bytes != bufferSize) {
      log_warning(SOCKET_LOGGER, "send: No coinciden los bytes enviados (%zd) con los que se esperaban enviar (%zd)\n", bufferSize, bytes);
      return 1;
  }

  return 0;
}

int package_receive(t_Package **destination, int fd_socket) {
  if(destination == NULL)
    return 1;

  *destination = package_create();

  if(package_receive_header(*destination, fd_socket))
    return 1;

  if(package_receive_payload(*destination, fd_socket))
    return 1;

  return 0;
}

int package_receive_header(t_Package *package, int fd_socket) {

  if(package == NULL)
    return 1;

  t_EnumValue aux;

  if(receive(fd_socket, (void *) &(aux), sizeof(aux)))
    return 1;

  package->header = (e_Header) aux;

  return 0;
}

int package_receive_payload(t_Package *package, int fd_socket) {

  if(package == NULL)
    return 1;

  t_Size aux;

  if(receive(fd_socket, (void *) &(aux), sizeof(aux)))
    return 1;

  package->payload.size = (size_t) aux;

  if(package->payload.size == 0)
    return 0;

  package->payload.stream = malloc((size_t) package->payload.size);
  if(package->payload.stream == NULL) {
    log_warning(SOCKET_LOGGER, "malloc: No se pudo reservar %zu bytes de memoria\n", (size_t) package->payload.size);
    return 1;
  }

  return receive(fd_socket, (void *) package->payload.stream, (size_t) package->payload.size);
}

int receive(int fd_socket, void *destination, size_t expected_bytes) {

  ssize_t bytes = recv(fd_socket, destination, expected_bytes, 0); // MSG_WAITALL
  if (bytes == 0) {
      log_warning(SOCKET_LOGGER, "recv: No hay mensajes disponibles para recibir y el par ha realizado un cierre ordenado\n");
      return 1;
  }
  if (bytes == -1) {
      log_warning(SOCKET_LOGGER, "recv: %s\n", strerror(errno));
      return 1;
  }
  if (bytes != expected_bytes) {
      log_warning(SOCKET_LOGGER, "recv: No coinciden los bytes recibidos (%zu) con los que se esperaban recibir (%zd)\n", expected_bytes, bytes);
      return 1;
  }

  return 0;
}