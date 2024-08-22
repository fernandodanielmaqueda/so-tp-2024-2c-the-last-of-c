/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_CPU_DISPATCH;
t_Server SERVER_CPU_INTERRUPT;

t_Client CLIENT_KERNEL_CPU_DISPATCH;
t_Client CLIENT_KERNEL_CPU_INTERRUPT;

t_Connection CONNECTION_MEMORY;

void initialize_sockets(void) {
    pthread_t thread_cpu_connect_to_memory;

    // [Server] CPU (Dispatch) <- [Cliente] Kernel
    pthread_create(&(SERVER_CPU_DISPATCH.thread_server), NULL, (void *(*)(void *)) cpu_start_server_for_kernel, (void *) &CLIENT_KERNEL_CPU_DISPATCH);
    // [Server] CPU (Interrupt) <- [Cliente] Kernel
    pthread_create(&(SERVER_CPU_INTERRUPT.thread_server), NULL, (void *(*)(void *)) cpu_start_server_for_kernel, (void *) &CLIENT_KERNEL_CPU_INTERRUPT);
    // [Client] CPU -> [Server] Memoria
    pthread_create(&thread_cpu_connect_to_memory, NULL, (void *(*)(void *)) client_thread_connect_to_server, (void *) &CONNECTION_MEMORY);

    // Se bloquea hasta que se realicen todas las conexiones
    pthread_join(SERVER_CPU_DISPATCH.thread_server, NULL);
    pthread_join(SERVER_CPU_INTERRUPT.thread_server, NULL);
    pthread_join(thread_cpu_connect_to_memory, NULL);
}

void finish_sockets(void) {
    close(CLIENT_KERNEL_CPU_DISPATCH.fd_client);
    close(CLIENT_KERNEL_CPU_INTERRUPT.fd_client);
    close(CONNECTION_MEMORY.fd_connection);
}

void *cpu_start_server_for_kernel(t_Client *new_client) {

    log_trace(MODULE_LOGGER, "Hilo de [Servidor] %s para [Cliente] %s iniciado", PORT_NAMES[new_client->server->server_type], PORT_NAMES[new_client->server->clients_type]);

    e_Port_Type port_type;
    int fd_new_client;

    server_start(new_client->server);

    while(1) {
        while(1) {
            log_trace(SOCKET_LOGGER, "Esperando [Cliente] %s en Puerto: %s", PORT_NAMES[new_client->server->clients_type], new_client->server->port);
            fd_new_client = server_accept(new_client->server->fd_listen);

            if (fd_new_client != -1)
                break;

            log_warning(SOCKET_LOGGER, "Fallo al aceptar [Cliente] %s en Puerto: %s", PORT_NAMES[new_client->server->clients_type], new_client->server->port);
        }

        log_trace(SOCKET_LOGGER, "Aceptado [Cliente] %s en Puerto: %s", PORT_NAMES[new_client->server->clients_type], new_client->server->port);

        // Handshake

        if(receive_port_type(&port_type, fd_new_client)) {
            log_warning(SOCKET_LOGGER, "Error al recibir Handshake de [Cliente]");
            close(fd_new_client);
            continue;
        }

        if (port_type != new_client->server->clients_type) {
            log_warning(SOCKET_LOGGER, "No reconocido Handshake de [Cliente]");
            send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, fd_new_client);
            close(fd_new_client);
            continue;
        }

        if(send_port_type(new_client->server->server_type, fd_new_client)) {
            log_warning(SOCKET_LOGGER, "Error al enviar Handshake a [Cliente] %s", PORT_NAMES[new_client->server->clients_type]);
            close(fd_new_client);
            continue;
        }

        log_debug(SOCKET_LOGGER, "OK Handshake con [Cliente] Kernel");
        break;
    }

    new_client->fd_client = fd_new_client;

    close(new_client->server->fd_listen);

    return NULL;
}