/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "scheduler.h"

pthread_rwlock_t SCHEDULING_RWLOCK;

t_Shared_List SHARED_LIST_NEW = { .list = NULL };

pthread_rwlock_t ARRAY_READY_RWLOCK;

t_Shared_List *ARRAY_LIST_READY = NULL;
t_Priority PRIORITY_COUNT = 0;

t_TCB *TCB_EXEC = NULL;
pthread_mutex_t MUTEX_EXEC;

t_Shared_List SHARED_LIST_BLOCKED_MEMORY_DUMP = { .list = NULL };
pthread_cond_t COND_BLOCKED_MEMORY_DUMP;

t_Shared_List SHARED_LIST_BLOCKED_IO_READY = { .list = NULL };

t_TCB *TCB_BLOCKED_IO_EXEC = NULL;
pthread_mutex_t MUTEX_BLOCKED_IO_EXEC;

t_Shared_List SHARED_LIST_EXIT = { .list = NULL };

t_Bool_Thread THREAD_LONG_TERM_SCHEDULER_NEW = { .running = false };
sem_t SEM_LONG_TERM_SCHEDULER_NEW;

t_Bool_Thread THREAD_LONG_TERM_SCHEDULER_EXIT = { .running = false };
sem_t SEM_LONG_TERM_SCHEDULER_EXIT;

bool IS_TCB_IN_CPU = false;
pthread_mutex_t MUTEX_IS_TCB_IN_CPU;
pthread_condattr_t CONDATTR_IS_TCB_IN_CPU;
pthread_cond_t COND_IS_TCB_IN_CPU;

t_Time QUANTUM;
t_Bool_Thread THREAD_QUANTUM_INTERRUPTER = { .running = false };
sem_t BINARY_QUANTUM_INTERRUPTER;

sem_t SEM_SHORT_TERM_SCHEDULER;
sem_t BINARY_SHORT_TERM_SCHEDULER;

bool CANCEL_IO_OPERATION = false;
pthread_mutex_t MUTEX_CANCEL_IO_OPERATION;
pthread_cond_t COND_CANCEL_IO_OPERATION;

t_Bool_Thread THREAD_IO_DEVICE = { .running = false };
sem_t SEM_IO_DEVICE;

bool FREE_MEMORY = 1;
pthread_mutex_t MUTEX_FREE_MEMORY;
pthread_cond_t COND_FREE_MEMORY;

bool KILL_EXEC_TCB;
e_Exit_Reason KILL_EXIT_REASON;

bool SHOULD_REDISPATCH = 0;

void initialize_scheduling(void) {
	int status;

	// Long term scheduler
	if((status = pthread_create(&THREAD_LONG_TERM_SCHEDULER_NEW.thread, NULL, (void *(*)(void *)) long_term_scheduler_new, NULL))) {
		log_error_pthread_create(status);
		error_pthread();
	}
	THREAD_LONG_TERM_SCHEDULER_NEW.running = true;

	if((status = pthread_create(&THREAD_LONG_TERM_SCHEDULER_EXIT.thread, NULL, (void *(*)(void *)) long_term_scheduler_exit, NULL))) {
		log_error_pthread_create(status);
		error_pthread();
	}
	THREAD_LONG_TERM_SCHEDULER_EXIT.running = true;

	// Quantum Interrupter (Push)
	switch(SCHEDULING_ALGORITHM) {

		case FIFO_SCHEDULING_ALGORITHM:
		case PRIORITIES_SCHEDULING_ALGORITHM:
			break;

		case MLQ_SCHEDULING_ALGORITHM:
			if((status = pthread_create(&THREAD_QUANTUM_INTERRUPTER.thread, NULL, (void *(*)(void *)) quantum_interrupter, NULL))) {
				log_error_pthread_create(status);
				error_pthread();
			}
			THREAD_QUANTUM_INTERRUPTER.running = true;
			break;
	}

	// IO Device
	if((status = pthread_create(&THREAD_IO_DEVICE.thread, NULL, (void *(*)(void *)) io_device, NULL))) {
		log_error_pthread_create(status);
		error_pthread();
	}
	THREAD_IO_DEVICE.running = true;
}

int finish_scheduling(void) {
	int retval = 0, status;

	t_Bool_Thread *threads[] = { &THREAD_LONG_TERM_SCHEDULER_NEW, &THREAD_LONG_TERM_SCHEDULER_EXIT, &THREAD_QUANTUM_INTERRUPTER, &THREAD_IO_DEVICE, NULL};
	register unsigned int i;

	for(i = 0; threads[i] != NULL; i++) {
		if(threads[i]->running) {
			if((status = pthread_cancel(threads[i]->thread))) {
				log_error_pthread_cancel(status);
				retval = -1;
			}
		}
	}

	for(i = 0; threads[i] != NULL; i++) {
		if(threads[i]->running) {
			if((status = pthread_join(threads[i]->thread, NULL))) {
				log_error_pthread_join(status);
				retval = -1;
			}
		}
	}

	return retval;
}

void *long_term_scheduler_new(void) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en NEW) iniciado");

	t_Connection connection_memory = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};
	t_PCB *pcb;
	int status, result;

	while(1) {

		if(wait_free_memory()) {
			error_pthread();
		}

		if(sem_wait(&SEM_LONG_TERM_SCHEDULER_NEW)) {
			log_error_sem_wait();
			error_pthread();
		}

		if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_rdlock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

			if(get_state_new(&pcb)) {
				error_pthread();
			}

		pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
		if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_unlock(status);
			error_pthread();
		}

		if(pcb == NULL) {
			continue;
		}

		pthread_cleanup_push((void (*)(void *)) reinsert_state_new, pcb);

			client_thread_connect_to_server(&connection_memory);
			pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_memory.fd_connection));

				if(send_process_create(pcb->PID, pcb->size, connection_memory.fd_connection)) {
					log_error(MODULE_LOGGER, "[%d] Error al enviar solicitud de creacion de proceso a [Servidor] %s [PID: %u - Tamaño: %zu]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID, pcb->size);
					error_pthread();
				}
				log_trace(MODULE_LOGGER, "[%d] Se envia solicitud de creacion de proceso a [Servidor] %s [PID: %u - Tamaño: %zu]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID, pcb->size);

				if(receive_result_with_expected_header(PROCESS_CREATE_HEADER, &result, connection_memory.fd_connection)) {
					log_error(MODULE_LOGGER, "[%d] Error al recibir resultado de creacion de proceso de [Servidor] %s [PID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID);
					error_pthread();
				}
				log_trace(MODULE_LOGGER, "[%d] Se recibe resultado de creacion de proceso de [Servidor] %s [PID: %u - Resultado: %d]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID, result);

			pthread_cleanup_pop(0);
			if(close(connection_memory.fd_connection)) {
				log_error_close();
				error_pthread();
			}

			if(result) {
				if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
					log_error_pthread_mutex_lock(status);
					error_pthread();
				}
					FREE_MEMORY = 0;
				if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
					log_error_pthread_mutex_unlock(status);
					error_pthread();
				}

				if(sem_post(&SEM_LONG_TERM_SCHEDULER_NEW)) {
					log_error_sem_post();
					error_pthread();
				}

				goto cleanup_pcb;
			}

			if(request_thread_create(pcb, ((t_TCB **) (pcb->thread_manager.array))[0]->TID)) {
				error_pthread();
			}

		cleanup_pcb:
		pthread_cleanup_pop(0); // reinsert_state_new
		if(result) {
			if(reinsert_state_new(pcb)) {
				error_pthread();
			}
			continue;
		}

		if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_rdlock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

			if(array_list_ready_update(((t_TCB **) (pcb->thread_manager.array))[0]->priority)) {
				error_pthread();
			}

			if(insert_state_ready(((t_TCB **) (pcb->thread_manager.array))[0])) {
				error_pthread();
			}

		pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
		if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_unlock(status);
			error_pthread();
		}
	}
}

void *long_term_scheduler_exit(void) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en EXIT) iniciado");

	t_Connection connection_memory = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};
	t_TCB *tcb;
	t_PCB *pcb;
	int status;

	while(1) {
		if(sem_wait(&SEM_LONG_TERM_SCHEDULER_EXIT)) {
			log_error_sem_wait();
			error_pthread();
		}

		if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_rdlock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

			if(get_state_exit(&tcb)) {
				error_pthread();
			}

		pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
		if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_unlock(status);
			error_pthread();
		}

		if(tcb == NULL) {
			continue;
		}

		log_info(MODULE_LOGGER, "Finaliza el hilo %u:%u - Motivo: %s", tcb->pcb->PID, tcb->TID, EXIT_REASONS[tcb->exit_reason]);

		client_thread_connect_to_server(&connection_memory);
		pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_memory.fd_connection));

			if(send_thread_destroy(tcb->pcb->PID, tcb->TID, connection_memory.fd_connection)) {
				log_error(MODULE_LOGGER, "[%d] Error al enviar solicitud de finalización de hilo a [Servidor] %s [PID: %u - TID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], tcb->pcb->PID, tcb->TID);
				error_pthread();
			}
			log_trace(MODULE_LOGGER, "[%d] Se envia solicitud de finalización de hilo a [Servidor] %s [PID: %u - TID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], tcb->pcb->PID, tcb->TID);

			if(receive_expected_header(THREAD_DESTROY_HEADER, connection_memory.fd_connection)) {
				log_error(MODULE_LOGGER, "[%d] Error al recibir confirmación de finalización de hilo de [Servidor] %s [PID: %u - TID %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], tcb->pcb->PID, tcb->TID);
				error_pthread();
			}
			log_trace(MODULE_LOGGER, "[%d] Se recibe confirmación de finalización de hilo de [Servidor] %s [PID: %u - TID %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], tcb->pcb->PID, tcb->TID);

		pthread_cleanup_pop(0);
		if(close(connection_memory.fd_connection)) {
			log_error_close();
			error_pthread();
		}

		pcb = tcb->pcb;

		resources_unassign(tcb);

		if(join_threads(tcb)) {
			error_pthread();
		}

		if(tid_release(&(tcb->pcb->thread_manager), tcb->TID)) {
			error_pthread();
		}
		if(tcb_destroy(tcb)) {
			error_pthread();
		}

		if((pcb->thread_manager.counter) == 0) {
			client_thread_connect_to_server(&connection_memory);
			pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_memory.fd_connection));

				if(send_process_destroy(tcb->pcb->PID, connection_memory.fd_connection)) {
					log_error(MODULE_LOGGER, "[%d] Error al enviar solicitud de finalización de proceso a [Servidor] %s [PID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID);
					error_pthread();
				}
				log_trace(MODULE_LOGGER, "[%d] Se envia solicitud de finalización de proceso a [Servidor] %s [PID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID);

				if(receive_expected_header(PROCESS_DESTROY_HEADER, connection_memory.fd_connection)) {
					log_error(MODULE_LOGGER, "[%d] Error al recibir confirmación de finalización de proceso de [Servidor] %s [PID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID);
					error_pthread();
				}
				log_trace(MODULE_LOGGER, "[%d] Se recibe confirmación de finalización de proceso de [Servidor] %s [PID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pcb->PID);

			pthread_cleanup_pop(0);
			if(close(connection_memory.fd_connection)) {
				log_error_close();
				error_pthread();
			}

			if(pid_release(&PID_MANAGER, pcb->PID)) {
				error_pthread();
			}
			if(pcb_destroy(pcb)) {
				error_pthread();
			}
		}
	}

	return NULL;
}

void *quantum_interrupter(void) {

	log_trace(MODULE_LOGGER, "Hilo de interrupciones de quantum iniciado");

	struct timespec ts_now, ts_quantum, ts_abstime;
	int retval = 0, status;

	while(1) {
		if(sem_wait(&BINARY_QUANTUM_INTERRUPTER)) {
			log_error_sem_wait();
			error_pthread();
		}

		if(clock_gettime(CLOCK_MONOTONIC_RAW, &ts_now)) {
			log_error_clock_gettime();
			error_pthread();
		}
		ts_quantum = timespec_from_ms(TCB_EXEC->quantum);

		ts_abstime = timespec_add(ts_now, ts_quantum);

		if((status = pthread_mutex_lock(&MUTEX_IS_TCB_IN_CPU))) {
			log_error_pthread_mutex_lock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_IS_TCB_IN_CPU);

			while(!IS_TCB_IN_CPU && status == 0) {
				status = pthread_cond_timedwait(&COND_IS_TCB_IN_CPU, &MUTEX_IS_TCB_IN_CPU, &ts_abstime);
			}
			switch(status) {
				case 0:
					// El hilo fue desalojado antes de que se agote el quantum
					retval = -1;
					break;
				case ETIMEDOUT:
					// Se agotó el quantum
					break;
				default:
					log_error_pthread_cond_timedwait(status);
					error_pthread();
			}

		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&MUTEX_IS_TCB_IN_CPU))) {
			log_error_pthread_mutex_unlock(status);
			error_pthread();
		}

		if(retval) {
			goto post_binary_short_term_scheduler;
		}

		// Envio la interrupción solo si hay más procesos en la cola de READY
		if(sem_wait(&SEM_SHORT_TERM_SCHEDULER)) {
			log_error_sem_wait();
			error_pthread();
		}
		if(sem_post(&SEM_SHORT_TERM_SCHEDULER)) {
			log_error_sem_post();
			error_pthread();
		}

		if((status = pthread_mutex_lock(&MUTEX_IS_TCB_IN_CPU))) {
			log_error_pthread_mutex_lock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_IS_TCB_IN_CPU);
			if(!IS_TCB_IN_CPU) {
				retval = -1;
			}
		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&MUTEX_IS_TCB_IN_CPU))) {
			log_error_pthread_mutex_unlock(status);
			error_pthread();
		}

		if(retval) {
			goto post_binary_short_term_scheduler;
		}

		if(send_kernel_interrupt(QUANTUM_KERNEL_INTERRUPT, TCB_EXEC->pcb->PID, TCB_EXEC->TID, CONNECTION_CPU_INTERRUPT.fd_connection)) {
			log_error(SOCKET_LOGGER, "[%d] Error al enviar interrupcion por quantum tras %li ms a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_INTERRUPT.fd_connection, TCB_EXEC->quantum, PORT_NAMES[CONNECTION_CPU_INTERRUPT.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
			error_pthread();
		}
		log_trace(SOCKET_LOGGER, "[%d] Se envia interrupcion por quantum tras %li ms a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_INTERRUPT.fd_connection, TCB_EXEC->quantum, PORT_NAMES[CONNECTION_CPU_INTERRUPT.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
	
		post_binary_short_term_scheduler:
		if(sem_post(&BINARY_SHORT_TERM_SCHEDULER)) {
			log_error_sem_post();
			error_pthread();
		}
	}

	return NULL;
}

void *short_term_scheduler(void) {

	log_trace(MODULE_LOGGER, "Hilo planificador de corto plazo iniciado");

	t_TCB *tcb;
	e_Eviction_Reason eviction_reason;
	t_Payload syscall_instruction;
	int status;

	while(1) {
		if(sem_wait(&SEM_SHORT_TERM_SCHEDULER)) {
			log_error_sem_wait();
			error_pthread();
		}

		if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_rdlock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

			KILL_EXEC_TCB = false;

			if(get_state_ready(&tcb)) {
				error_pthread();
			}

			if(tcb == NULL) {
				goto cleanup_scheduling_rwlock;
			}

			if(insert_state_exec(tcb)) {
				error_pthread();
			}

			if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, TCB_EXEC->pcb->PID, TCB_EXEC->TID, CONNECTION_CPU_DISPATCH.fd_connection)) {
				log_error(MODULE_LOGGER, "[%d] Error al enviar dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.fd_connection, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
				error_pthread();
			}
			log_trace(MODULE_LOGGER, "[%d] Se envia dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.fd_connection, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);

		cleanup_scheduling_rwlock:
		pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
		if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_unlock(status);
			error_pthread();
		}

		if(tcb == NULL) {
			continue;
		}

		SHOULD_REDISPATCH = 1;
		do {

			if((status = pthread_mutex_lock(&MUTEX_IS_TCB_IN_CPU))) {
				log_error_pthread_mutex_lock(status);
				error_pthread();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_IS_TCB_IN_CPU);
				IS_TCB_IN_CPU = true;
			pthread_cleanup_pop(0);
			if((status = pthread_mutex_unlock(&MUTEX_IS_TCB_IN_CPU))) {
				log_error_pthread_mutex_unlock(status);
				error_pthread();
			}

			switch(SCHEDULING_ALGORITHM) {

				case FIFO_SCHEDULING_ALGORITHM:
				case PRIORITIES_SCHEDULING_ALGORITHM:
					break;

				case MLQ_SCHEDULING_ALGORITHM:
					if(sem_post(&BINARY_QUANTUM_INTERRUPTER)) {
						log_error_sem_post();
						error_pthread();
					}
					break;

			}

			payload_init(&syscall_instruction);
			pthread_cleanup_push((void (*)(void *)) payload_destroy, &syscall_instruction);

			if(receive_thread_eviction(&eviction_reason, &syscall_instruction, CONNECTION_CPU_DISPATCH.fd_connection)) {
				log_error(MODULE_LOGGER, "[%d] Error al recibir desalojo de hilo de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.fd_connection, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
				error_pthread();
			}
			log_trace(MODULE_LOGGER, "[%d] Se recibe desalojo de hilo de [Servidor] %s [PID: %u - TID: %u - Motivo: %s]", CONNECTION_CPU_DISPATCH.fd_connection, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID, EVICTION_REASON_NAMES[eviction_reason]);

			switch(SCHEDULING_ALGORITHM) {

				case FIFO_SCHEDULING_ALGORITHM:
				case PRIORITIES_SCHEDULING_ALGORITHM:
					break;

				case MLQ_SCHEDULING_ALGORITHM:

					if((status = pthread_mutex_lock(&MUTEX_IS_TCB_IN_CPU))) {
						log_error_pthread_mutex_lock(status);
						error_pthread();
					}
					pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_IS_TCB_IN_CPU);
						IS_TCB_IN_CPU = false;
						pthread_cond_signal(&COND_IS_TCB_IN_CPU);
					pthread_cleanup_pop(0);
					if((status = pthread_mutex_unlock(&MUTEX_IS_TCB_IN_CPU))) {
						log_error_pthread_mutex_unlock(status);
						error_pthread();
					}

					if(sem_post(&SEM_SHORT_TERM_SCHEDULER)) {
						log_error_sem_post();
						error_pthread();
					}
					pthread_cleanup_push((void (*)(void *)) sem_wait, &SEM_SHORT_TERM_SCHEDULER);
						if(sem_wait(&BINARY_SHORT_TERM_SCHEDULER)) {
							log_error_sem_wait();
							error_pthread();
						}
					pthread_cleanup_pop(0);
					if(sem_wait(&SEM_SHORT_TERM_SCHEDULER)) {
						log_error_sem_wait();
						error_pthread();
					}

					break;

			}

			if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
				log_error_pthread_rwlock_rdlock(status);
				error_pthread();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

			switch(eviction_reason) {
				case UNEXPECTED_ERROR_EVICTION_REASON:
					TCB_EXEC->exit_reason = UNEXPECTED_ERROR_EXIT_REASON;
					if(get_state_exec(&TCB_EXEC)) {
						error_pthread();
					}
					if(insert_state_exit(TCB_EXEC)) {
						error_pthread();
					}
					SHOULD_REDISPATCH = 0;
					break;

				case SEGMENTATION_FAULT_EVICTION_REASON:
					TCB_EXEC->exit_reason = SEGMENTATION_FAULT_EXIT_REASON;
					if(get_state_exec(&TCB_EXEC)) {
						error_pthread();
					}
					if(insert_state_exit(TCB_EXEC)) {
						error_pthread();
					}
					SHOULD_REDISPATCH = 0;
					break;

				case EXIT_EVICTION_REASON:
					if(syscall_execute(&syscall_instruction)) {
						// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC)
						if(get_state_exec(&TCB_EXEC)) {
							error_pthread();
						}
						if(insert_state_exit(TCB_EXEC)) {
							error_pthread();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC) y/o el SHOULD_REDISPATCH
					break;

				case KILL_EVICTION_REASON:
					TCB_EXEC->exit_reason = KILL_EXIT_REASON;
					if(get_state_exec(&TCB_EXEC)) {
						error_pthread();
					}
					if(insert_state_exit(TCB_EXEC)) {
						error_pthread();
					}
					SHOULD_REDISPATCH = 0;
					break;
					
				case SYSCALL_EVICTION_REASON:

					if(KILL_EXEC_TCB) {
						TCB_EXEC->exit_reason = KILL_EXIT_REASON;
						if(get_state_exec(&TCB_EXEC)) {
							error_pthread();
						}
						if(insert_state_exit(TCB_EXEC)) {
							error_pthread();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					if(syscall_execute(&syscall_instruction)) {
						// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC)
						if(get_state_exec(&TCB_EXEC)) {
							error_pthread();
						}
						if(insert_state_exit(TCB_EXEC)) {
							error_pthread();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC) y/o el SHOULD_REDISPATCH
					break;

				case QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON:
					log_info(MINIMAL_LOGGER, "## (%u:%u) - Desalojado por fin de Quantum", TCB_EXEC->pcb->PID, TCB_EXEC->TID);

					if(KILL_EXEC_TCB) {
						TCB_EXEC->exit_reason = KILL_EXIT_REASON;
						if(get_state_exec(&TCB_EXEC)) {
							error_pthread();
						}
						if(insert_state_exit(TCB_EXEC)) {
							error_pthread();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					if(get_state_exec(&TCB_EXEC)) {
						error_pthread();
					}
					if(insert_state_ready(TCB_EXEC)) {
						error_pthread();
					}
					SHOULD_REDISPATCH = 0;
					break;
			}

			if(SHOULD_REDISPATCH) {
				if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, TCB_EXEC->pcb->PID, TCB_EXEC->TID, CONNECTION_CPU_DISPATCH.fd_connection)) {
					log_error(MODULE_LOGGER, "[%d] Error al enviar dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.fd_connection, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
					error_pthread();
				}
				log_trace(MODULE_LOGGER, "[%d] Se envia dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.fd_connection, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
			}

			pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
			if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
				log_error_pthread_rwlock_unlock(status);
				error_pthread();
			}

			pthread_cleanup_pop(1); // syscall_instruction
		} while(SHOULD_REDISPATCH);
	}

	error_pthread();
}

void *io_device(void) {

	log_trace(MODULE_LOGGER, "Hilo de IO iniciado");

	t_TCB *tcb;
	t_Time time;
	struct timespec ts_now, ts_time, ts_abstime;
	int status;

	while(1) {
		if(sem_wait(&SEM_IO_DEVICE)) {
			log_error_sem_wait();
			error_pthread();
		}

		if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_rdlock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

			if(get_state_blocked_io_ready(&tcb)) {
				error_pthread();
			}

			if(tcb == NULL) {
				goto cleanup_scheduling_rwlock;
			}

			if(insert_state_blocked_io_exec(tcb)) {
				error_pthread();
			}

			payload_remove(&(TCB_BLOCKED_IO_EXEC->syscall_instruction), &time, sizeof(time));

			CANCEL_IO_OPERATION = false;

		cleanup_scheduling_rwlock:
		pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
		if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_unlock(status);
			error_pthread();
		}


		if(tcb == NULL) {
			continue;
		}

		if(clock_gettime(CLOCK_MONOTONIC_RAW, &ts_now)) {
			log_error_clock_gettime();
			error_pthread();
		}
		ts_time = timespec_from_ms(time);

		ts_abstime = timespec_add(ts_now, ts_time);

		if((status = pthread_mutex_lock(&MUTEX_CANCEL_IO_OPERATION))) {
			log_error_pthread_mutex_lock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_CANCEL_IO_OPERATION);

			while(!CANCEL_IO_OPERATION && status == 0) {
				status = pthread_cond_timedwait(&COND_CANCEL_IO_OPERATION, &MUTEX_CANCEL_IO_OPERATION, &ts_abstime);
			}
			switch(status) {
				case 0:
					// La operación de entrada/salida fue cancelada
					break;
				case ETIMEDOUT:
					// Se terminó la operación de entrada/salida
					break;
				default:
					log_error_pthread_cond_timedwait(status);
					error_pthread();
			}

		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&MUTEX_CANCEL_IO_OPERATION))) {
			log_error_pthread_mutex_unlock(status);
			error_pthread();
		}


		if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_rdlock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

			if(get_state_blocked_io_exec(&tcb)) {
				error_pthread();
			}

			if(tcb != NULL) {
				log_info(MINIMAL_LOGGER, "## (%u:%u) finalizó IO y pasa a READY", tcb->pcb->PID, tcb->TID);
				if(insert_state_ready(tcb)) {
					error_pthread();
				}
			}

		pthread_cleanup_pop(0);
		if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
			log_error_pthread_rwlock_unlock(status);
			error_pthread();
		}
	}

	error_pthread();
}

int wait_free_memory(void) {
	int retval = 0, status;

	if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_FREE_MEMORY);
		while(!FREE_MEMORY) {
			if((status = pthread_cond_wait(&COND_FREE_MEMORY, &MUTEX_FREE_MEMORY))) {
				log_error_pthread_cond_wait(status);
				retval = -1;
				break;
			}
		}
	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_unlock(status);
		retval = -1;
		goto ret;
	}

	ret:
	return retval;
}

void *dump_memory_petitioner(void) {
	pthread_t thread = pthread_self();
	int retval = 0, status, result;
	t_PID pid;
	t_TID tid;

	pthread_cleanup_push((void (*)(void *)) remove_dump_memory_thread, &thread);

	log_trace(MODULE_LOGGER, "Hilo de petición de volcado de memoria iniciado");

    if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_rdlock(status);
        error_pthread();
    }
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

		if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
			log_error_pthread_mutex_lock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex));

			t_Dump_Memory_Petition *dump_memory_petition = list_find_by_condition_with_comparation(SHARED_LIST_BLOCKED_MEMORY_DUMP.list, (bool (*)(void *, void *)) dump_memory_petition_matches_pthread, &thread);
			if((dump_memory_petition == NULL) || (dump_memory_petition->tcb == NULL)) {
				retval = -1;
				goto cleanup_shared_list_blocked_memory_dump_mutex;
			}

			pid = dump_memory_petition->tcb->pcb->PID;
			tid = dump_memory_petition->tcb->TID;
		
		cleanup_shared_list_blocked_memory_dump_mutex:
		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
			log_error_pthread_mutex_unlock(status);
			error_pthread();
		}

	pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
	if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
		log_error_pthread_rwlock_unlock(status);
		error_pthread();
	}

	if(retval) {
		goto cleanup_remove_dump_memory_thread;
	}

	t_Connection connection_memory = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};

	client_thread_connect_to_server(&connection_memory);
	pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_memory.fd_connection));

		if(send_pid_and_tid_with_header(MEMORY_DUMP_HEADER, pid, tid, connection_memory.fd_connection)) {
			log_error(MODULE_LOGGER, "[%d] Error al enviar solicitud de volcado de memoria a [Servidor] %s [PID: %u - TID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pid, tid);
			error_pthread();
		}
		log_trace(MODULE_LOGGER, "[%d] Se envia solicitud de volcado de memoria a [Servidor] %s [PID: %u - TID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pid, tid);

		if(receive_result_with_expected_header(MEMORY_DUMP_HEADER, &result, connection_memory.fd_connection)) {
			log_error(MODULE_LOGGER, "[%d] Error al recibir resultado de volcado de memoria de [Servidor] %s [PID: %u - TID: %u]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pid, tid);
			error_pthread();
		}
		log_trace(MODULE_LOGGER, "[%d] Se recibe resultado de volcado de memoria de [Servidor] %s [PID: %u - TID: %u - Resultado: %d]", connection_memory.fd_connection, PORT_NAMES[connection_memory.server_type], pid, tid, result);

	pthread_cleanup_pop(0);
	if(close(connection_memory.fd_connection)) {
		log_error_close();
		error_pthread();
	}

	cleanup_remove_dump_memory_thread:
	pthread_cleanup_pop(0);
	if(remove_dump_memory_thread(&thread)) {
		error_pthread();
	}

	error_pthread();
}

int remove_dump_memory_thread(pthread_t *thread) {
	int retval = 0, status;
	t_Dump_Memory_Petition *dump_memory_petition = NULL;

    if((status = pthread_rwlock_rdlock(&SCHEDULING_RWLOCK))) {
        log_error_pthread_rwlock_rdlock(status);
        return -1;
    }
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &SCHEDULING_RWLOCK);

		pthread_cleanup_push((void (*)(void *)) free, dump_memory_petition);

			if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto cleanup_scheduling_rwlock;
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex));

				dump_memory_petition = list_remove_by_condition_with_comparation(SHARED_LIST_BLOCKED_MEMORY_DUMP.list, (bool (*)(void *, void *)) dump_memory_petition_matches_pthread, &thread);
				if(dump_memory_petition == NULL) {
					goto cleanup_shared_list_blocked_memory_dump_mutex;
				}

				if(SHARED_LIST_BLOCKED_MEMORY_DUMP.list->head == NULL) {
					if((status = pthread_cond_signal(&COND_BLOCKED_MEMORY_DUMP))) {
						log_error_pthread_cond_signal(status);
						retval = -1;
						goto cleanup_shared_list_blocked_memory_dump_mutex;
					}
				}

			cleanup_shared_list_blocked_memory_dump_mutex:
			pthread_cleanup_pop(0); // SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex
			if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
				log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto cleanup_dump_memory_petition;
			}

			if((retval) || (dump_memory_petition == NULL) || ((dump_memory_petition->tcb) == NULL)) {
				goto cleanup_dump_memory_petition;
			}

			if(insert_state_ready(dump_memory_petition->tcb)) {
				retval = -1;
				goto cleanup_dump_memory_petition;
			}

		cleanup_dump_memory_petition:
		pthread_cleanup_pop(1); // dump_memory_petition

	cleanup_scheduling_rwlock:
	pthread_cleanup_pop(0); // SCHEDULING_RWLOCK
	if((status = pthread_rwlock_unlock(&SCHEDULING_RWLOCK))) {
		log_error_pthread_rwlock_unlock(status);
		retval = -1;
	}

	return retval;
}

bool dump_memory_petition_matches_pthread(t_Dump_Memory_Petition *dump_memory_petition, pthread_t *thread) {
    return pthread_equal(dump_memory_petition->bool_thread.thread, *thread);
}

bool dump_memory_petition_matches_tcb(t_Dump_Memory_Petition *dump_memory_petition, t_TCB *tcb) {
    return dump_memory_petition->tcb == tcb;
}

int signal_free_memory(void) {
	int retval = 0, status;
	
	if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_FREE_MEMORY);
		FREE_MEMORY = 1;
		if((status = pthread_cond_signal(&COND_FREE_MEMORY))) {
			log_error_pthread_cond_signal(status);
			retval = -1;
		}
	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_unlock(status);
		retval = -1;
		goto ret;
	}

	ret:
	return retval;
}