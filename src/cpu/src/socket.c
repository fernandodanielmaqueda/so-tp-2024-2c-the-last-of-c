/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "socket.h"

t_Server SERVER_CPU_DISPATCH;
t_Server SERVER_CPU_INTERRUPT;

t_Client CLIENT_KERNEL_CPU_DISPATCH;
t_Client CLIENT_KERNEL_CPU_INTERRUPT;

t_Connection CONNECTION_MEMORY;

int initialize_sockets(void) {
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

    return 0;
}

void finish_sockets(void) {
    close(CLIENT_KERNEL_CPU_DISPATCH.fd_client);
    close(CLIENT_KERNEL_CPU_INTERRUPT.fd_client);
    close(CONNECTION_MEMORY.fd_connection);
}

void *cpu_start_server_for_kernel(t_Client *new_client) {

    log_trace(MODULE_LOGGER, "Hilo de [Servidor] %s para [Cliente] %s iniciado", PORT_NAMES[new_client->server->server_type], PORT_NAMES[new_client->server->clients_type]);

    e_Port_Type port_type;

    server_start(new_client->server);

    while(1) {
        while(1) {
            log_trace(SOCKET_LOGGER, "[%d] Esperando [Cliente] %s en Puerto: %s", new_client->server->fd_listen, PORT_NAMES[new_client->server->clients_type], new_client->server->port);
            new_client->fd_client = server_accept(new_client->server->fd_listen);

            if (new_client->fd_client != -1)
                break;

            log_warning(SOCKET_LOGGER, "[%d] Fallo al aceptar [Cliente] %s", new_client->server->fd_listen, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
        }

        log_trace(SOCKET_LOGGER, "[%d] Aceptado [Cliente] %s [%d]", new_client->server->fd_listen, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE], new_client->fd_client);

        // Handshake

        if(receive_port_type(&port_type, new_client->fd_client)) {
            log_warning(SOCKET_LOGGER, "[%d] Error al recibir Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
            close(new_client->fd_client);
            continue;
        }

        if(port_type != new_client->server->clients_type) {
            log_warning(SOCKET_LOGGER, "[%d] No reconocido Handshake de [Cliente] %s", new_client->fd_client, PORT_NAMES[TO_BE_IDENTIFIED_PORT_TYPE]);
            send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);
            close(new_client->fd_client);
            continue;
        }

        if(send_port_type(new_client->server->server_type, new_client->fd_client)) {
            log_warning(SOCKET_LOGGER, "[%d] Error al enviar Handshake a [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->server->clients_type]);
            close(new_client->fd_client);
            continue;
        }

        log_debug(SOCKET_LOGGER, "[%d] OK Handshake con [Cliente] %s", new_client->fd_client, PORT_NAMES[new_client->server->clients_type]);
        break;
    }

    log_trace(SOCKET_LOGGER, "[%d] Cerrando [Servidor] %s para Cliente [%s] en Puerto: %s", new_client->server->fd_listen, PORT_NAMES[new_client->server->server_type], PORT_NAMES[new_client->server->clients_type], new_client->server->port);
    close(new_client->server->fd_listen);

    return NULL;
}