/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "interfaces.h"

t_list *LIST_INTERFACES;
t_Drain_Ongoing_Resource_Sync INTERFACES_SYNC;

void *kernel_client_handler_for_io(t_Client *new_client) {

    log_trace(MODULE_LOGGER, "Hilo receptor de [Interfaz] de Entrada/Salida iniciado");

	e_Port_Type port_type;

	if(receive_port_type(&port_type, new_client->fd_client)) {
        log_warning(SOCKET_LOGGER, "Error al recibir Handshake de [Cliente]");
        close(new_client->fd_client);
        return NULL;
    }

	if(port_type != IO_PORT_TYPE) {
		log_warning(SOCKET_LOGGER, "No reconocido Handshake de [Cliente]");
		send_port_type(TO_BE_IDENTIFIED_PORT_TYPE, new_client->fd_client);
		close(new_client->fd_client);
		return NULL;
	}

    if(send_port_type(KERNEL_PORT_TYPE, new_client->fd_client)) {
        log_warning(SOCKET_LOGGER, "Error al enviar Handshake a [Cliente] Entrada/Salida");
        close(new_client->fd_client);
        return NULL;
    }
    
	log_debug(SOCKET_LOGGER, "OK Handshake con [Cliente] Entrada/Salida");

	t_Interface *interface = interface_create(new_client);

    pthread_create(&(interface->thread_io_interface_dispatcher), NULL, (void *(*)(void *)) kernel_io_interface_dispatcher, (void *) interface);

    wait_draining_requests(&SCHEDULING_SYNC);
        wait_ongoing_locking(&INTERFACES_SYNC);

            if(list_add_unless_any(LIST_INTERFACES, interface, (bool (*)(void *, void *)) interface_name_matches, interface->name)) {
                log_warning(MODULE_LOGGER, "Error al agregar [Interfaz] Entrada/Salida: Nombre de interfaz ya existente");
                send_return_value_with_header(INTERFACE_DATA_REQUEST_HEADER, 1, new_client->fd_client);
                interface_destroy(interface);
                return NULL;
            }

            if(send_return_value_with_header(INTERFACE_DATA_REQUEST_HEADER, 0, interface->client->fd_client)) {
                // TODO
                exit(1);
            }

        signal_ongoing_unlocking(&INTERFACES_SYNC);
    signal_draining_requests(&SCHEDULING_SYNC);
	
    t_PID pid;
    t_Return_Value return_value;

    t_PCB *pcb;

	while(1) {

        if(receive_io_operation_finished(&pid, &return_value, interface->client->fd_client)) {
            log_warning(MODULE_LOGGER, "Terminada conexion con [Cliente] Entrada/Salida: %s", interface->name);
            pthread_cancel(interface->thread_io_interface_dispatcher);
            pthread_join(interface->thread_io_interface_dispatcher, NULL);

            wait_draining_requests(&SCHEDULING_SYNC);

                wait_ongoing_locking(&INTERFACES_SYNC);
                    list_remove_by_condition_with_comparation(LIST_INTERFACES, (bool (*)(void *, void *)) interface_name_matches, interface->name);
                signal_ongoing_unlocking(&INTERFACES_SYNC);

                interface_exit(interface);
                interface_destroy(interface);
                
            signal_draining_requests(&SCHEDULING_SYNC);
			return NULL;
        }

        wait_draining_requests(&SCHEDULING_SYNC);
            wait_draining_requests(&INTERFACES_SYNC);
                pthread_mutex_lock(&(interface->shared_list_blocked_exec.mutex));

                    pcb = (t_PCB *) list_remove_by_condition_with_comparation(interface->shared_list_blocked_exec.list, (bool (*)(void *, void *)) pcb_matches_pid, &(pid));
                    if(pcb != NULL) {
                        pcb->shared_list_state = NULL;
                        
                        payload_destroy(&(pcb->io_operation));

                        if(return_value) {
                            pcb->exit_reason = UNEXPECTED_ERROR_EXIT_REASON;
                            switch_process_state(pcb, EXIT_STATE);
                        }
                        else {
                            switch_process_state(pcb, READY_STATE);
                        }
                        
                    }

                    sem_post(&(interface->sem_concurrency));

                pthread_mutex_unlock(&(interface->shared_list_blocked_exec.mutex));
            signal_draining_requests(&INTERFACES_SYNC);
        signal_draining_requests(&SCHEDULING_SYNC);
	}

	return NULL;
}

void *kernel_io_interface_dispatcher(t_Interface *interface) {

    log_trace(MODULE_LOGGER, "Hilo emisor de [Interfaz] de Entrada/Salida iniciado");

    t_PCB *pcb;

    while(1) {
        sem_wait(&(interface->sem_scheduler));
        pthread_cleanup_push((void (*)(void *)) sem_post, (void *) &(interface->sem_scheduler));
        sem_wait(&(interface->sem_concurrency));
        pthread_cleanup_push((void (*)(void *)) sem_post, (void *) &(interface->sem_concurrency));

        wait_draining_requests(&SCHEDULING_SYNC);
        pthread_cleanup_push((void (*)(void *)) signal_draining_requests, (void *) &SCHEDULING_SYNC);
            wait_draining_requests(&INTERFACES_SYNC);
            pthread_cleanup_push((void (*)(void *)) signal_draining_requests, (void *) &INTERFACES_SYNC);

                int empty = 0;
                pthread_mutex_lock(&(interface->shared_list_blocked_ready.mutex));
                pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(interface->shared_list_blocked_ready.mutex));
                    if((interface->shared_list_blocked_ready.list)->head == NULL) {
                        empty = 1;
                    }
                    else {
                        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
                        pcb = (t_PCB *) list_remove(interface->shared_list_blocked_ready.list, 0);
                        pcb->shared_list_state = NULL;
                    }
                pthread_cleanup_pop(1);

                if(!empty) {
                    pthread_mutex_lock(&(interface->shared_list_blocked_exec.mutex));
                    pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, (void *) &(interface->shared_list_blocked_exec.mutex));
                        list_add(interface->shared_list_blocked_exec.list, pcb);
                        pcb->shared_list_state = &(interface->shared_list_blocked_exec);
                    pthread_cleanup_pop(1);

                    if(send_io_operation_dispatch(pcb->exec_context.PID, pcb->io_operation, interface->client->fd_client)) {
                        log_warning(MODULE_LOGGER, "Error al enviar operacion de [Interfaz] Entrada/Salida");
                        pthread_exit(NULL);
                    }

                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                }

            pthread_cleanup_pop(1);
        pthread_cleanup_pop(1);
        
        pthread_cleanup_pop(0);
        pthread_cleanup_pop(0);
    }

    return NULL;
}

t_Interface *interface_create(t_Client *client) {
	
    t_Interface *interface = malloc(sizeof(t_Interface));
    if(interface == NULL) {
        log_error(MODULE_LOGGER, "malloc: Error al reservar memoria para [Interfaz] Entrada/Salida");
        return NULL;
    }
	
    interface->client = client;

	if(send_header(INTERFACE_DATA_REQUEST_HEADER, interface->client->fd_client)) {
        // TODO
        exit(1);
    }
	if(receive_interface_data(&(interface->name), &(interface->io_type), interface->client->fd_client)) {
        // TODO
        exit(1);
    }

    interface->shared_list_blocked_ready.list = list_create();
    pthread_mutex_init(&(interface->shared_list_blocked_ready.mutex), NULL);

    interface->shared_list_blocked_exec.list = list_create();
    pthread_mutex_init(&(interface->shared_list_blocked_exec.mutex), NULL);

    sem_init(&(interface->sem_scheduler), 0, 0);
    sem_init(&(interface->sem_concurrency), 0, 1);

    return interface;
}

void interface_exit(t_Interface *interface) {
    t_PCB *pcb;

    pthread_mutex_lock(&(interface->shared_list_blocked_ready.mutex));
        while((interface->shared_list_blocked_ready.list)->elements_count > 0) {
            pcb = (t_PCB *) list_remove(interface->shared_list_blocked_ready.list, 0);
            pcb->shared_list_state = NULL;
            payload_destroy(&(pcb->io_operation));
            pcb->exit_reason = INVALID_INTERFACE_EXIT_REASON;
            switch_process_state(pcb, EXIT_STATE);
        }
    pthread_mutex_unlock(&(interface->shared_list_blocked_exec.mutex));

    pthread_mutex_lock(&(interface->shared_list_blocked_exec.mutex));
        while((interface->shared_list_blocked_exec.list)->elements_count > 0) {
            pcb = (t_PCB *) list_remove(interface->shared_list_blocked_exec.list, 0);
            pcb->shared_list_state = NULL;
            payload_destroy(&(pcb->io_operation));
            pcb->exit_reason = INVALID_INTERFACE_EXIT_REASON;
            switch_process_state(pcb, EXIT_STATE);
        }
    pthread_mutex_unlock(&(interface->shared_list_blocked_exec.mutex));

}

void interface_destroy(t_Interface *interface) {
    close(interface->client->fd_client);
    free(interface->client);

    free(interface->name);

    list_destroy_and_destroy_elements(interface->shared_list_blocked_ready.list, (void (*)(void *)) pcb_destroy);
    list_destroy_and_destroy_elements(interface->shared_list_blocked_exec.list, (void (*)(void *)) pcb_destroy);

    pthread_mutex_destroy(&(interface->shared_list_blocked_ready.mutex));
    pthread_mutex_destroy(&(interface->shared_list_blocked_exec.mutex));

    sem_destroy(&(interface->sem_scheduler));
    sem_destroy(&(interface->sem_concurrency));

	free(interface);
}

bool interface_name_matches(t_Interface *interface, char *name) {
	return (strcmp(interface->name, name) == 0);
}