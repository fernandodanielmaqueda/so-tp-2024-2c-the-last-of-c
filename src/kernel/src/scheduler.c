/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "scheduler.h"

pthread_rwlock_t RWLOCK_SCHEDULING;

t_Shared_List SHARED_LIST_NEW = { .list = NULL };

pthread_rwlock_t RWLOCK_ARRAY_READY;

t_Ready **ARRAY_READY = NULL;
t_Priority PRIORITY_COUNT = 0;

t_TCB *TCB_EXEC = NULL;
pthread_mutex_t MUTEX_EXEC;
sem_t *SEM_READY;

t_Shared_List SHARED_LIST_BLOCKED_DUMP_MEMORY = { .list = NULL };
pthread_cond_t COND_BLOCKED_DUMP_MEMORY;

t_Shared_List SHARED_LIST_BLOCKED_IO_READY = { .list = NULL };

t_TCB *TCB_BLOCKED_IO_EXEC = NULL;
pthread_mutex_t MUTEX_BLOCKED_IO_EXEC;

t_Bool_Thread THREAD_LONG_TERM_SCHEDULER_NEW = { .running = false };
sem_t SEM_LONG_TERM_SCHEDULER_NEW;

t_Time QUANTUM = 0;
t_Bool_Thread THREAD_QUANTUM_INTERRUPTER = { .running = false };
sem_t BINARY_QUANTUM_INTERRUPTER;
pthread_mutex_t MUTEX_QUANTUM_INTERRUPTER;
pthread_condattr_t CONDATTR_QUANTUM_INTERRUPTER;
pthread_cond_t COND_QUANTUM_INTERRUPTER;
bool FINISH_QUANTUM;
bool QUANTUM_EXPIRED;
bool IS_TCB_IN_CPU;

sem_t SEM_SHORT_TERM_SCHEDULER;
sem_t BINARY_SHORT_TERM_SCHEDULER;

bool CANCEL_IO_OPERATION = false;
pthread_mutex_t MUTEX_CANCEL_IO_OPERATION;
pthread_condattr_t CONDATTR_CANCEL_IO_OPERATION;
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
		report_error_pthread_create(status);
		exit_sigint();
	}
	THREAD_LONG_TERM_SCHEDULER_NEW.running = true;

	if((status = pthread_create(&THREAD_QUANTUM_INTERRUPTER.thread, NULL, (void *(*)(void *)) quantum_interrupter, NULL))) {
		report_error_pthread_create(status);
		exit_sigint();
	}
	THREAD_QUANTUM_INTERRUPTER.running = true;

	// IO Device
	if((status = pthread_create(&THREAD_IO_DEVICE.thread, NULL, (void *(*)(void *)) io_device, NULL))) {
		report_error_pthread_create(status);
		exit_sigint();
	}
	THREAD_IO_DEVICE.running = true;
}

int finish_scheduling(void) {
	int retval = 0, status;

	t_Bool_Thread *threads[] = { &THREAD_LONG_TERM_SCHEDULER_NEW, &THREAD_QUANTUM_INTERRUPTER, &THREAD_IO_DEVICE, NULL};
	register unsigned int i;

	for(i = 0; threads[i] != NULL; i++) {
		if(threads[i]->running) {
			if((status = pthread_cancel(threads[i]->thread))) {
				report_error_pthread_cancel(status);
				retval = -1;
			}
		}
	}

	for(i = 0; threads[i] != NULL; i++) {
		if(threads[i]->running) {
			if((status = pthread_join(threads[i]->thread, NULL))) {
				report_error_pthread_join(status);
				retval = -1;
			}
		}
	}

	return retval;
}

void *long_term_scheduler_new(void) {

	log_trace_r(&MODULE_LOGGER, "Hilo planificador de largo plazo (en NEW) iniciado");

	t_Connection connection_memory = CONNECTION_MEMORY_INITIALIZER;
	t_PCB *pcb;
	int status, result;
	bool process_created;

	while(1) {

		process_created = false;

		if(wait_free_memory()) {
			exit_sigint();
		}

		if(sem_wait(&SEM_LONG_TERM_SCHEDULER_NEW)) {
			report_error_sem_wait();
			exit_sigint();
		}

		if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_rdlock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

			if(get_state_new(&pcb)) {
				exit_sigint();
			}

		pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
		if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_unlock(status);
			exit_sigint();
		}

		if(pcb == NULL) {
			continue;
		}

		pthread_cleanup_push((void (*)(void *)) reinsert_state_new, pcb);

			client_thread_connect_to_server(&connection_memory);
			pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_memory.socket_connection.fd));

				if(send_process_create(pcb->PID, pcb->size, connection_memory.socket_connection.fd)) {
					log_error_r(&MODULE_LOGGER, "[%d] Error al enviar solicitud de creación de proceso a [Servidor] %s [PID: %u - Tamaño: %zu]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID, pcb->size);
					exit_sigint();
				}
				log_trace_r(&MODULE_LOGGER, "[%d] Se envía solicitud de creación de proceso a [Servidor] %s [PID: %u - Tamaño: %zu]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID, pcb->size);

				if(receive_result_with_expected_header(PROCESS_CREATE_HEADER, &result, connection_memory.socket_connection.fd)) {
					log_error_r(&MODULE_LOGGER, "[%d] Error al recibir resultado de creación de proceso de [Servidor] %s [PID: %u]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID);
					exit_sigint();
				}
				log_trace_r(&MODULE_LOGGER, "[%d] Se recibe resultado de creación de proceso de [Servidor] %s [PID: %u - Resultado: %d]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], pcb->PID, result);

			pthread_cleanup_pop(0);
			if(close(connection_memory.socket_connection.fd)) {
				report_error_close();
				exit_sigint();
			}

			if(result) {
				if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
					report_error_pthread_mutex_lock(status);
					exit_sigint();
				}
					FREE_MEMORY = 0;
				if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
					report_error_pthread_mutex_unlock(status);
					exit_sigint();
				}

				if(sem_post(&SEM_LONG_TERM_SCHEDULER_NEW)) {
					report_error_sem_post();
					exit_sigint();
				}

				goto cleanup_pcb;
			}

			process_created = true;

			if(request_thread_create(pcb, ((t_TCB **) (pcb->thread_manager.array))[0]->TID, &result)) {
				exit_sigint();
			}

		cleanup_pcb:
		pthread_cleanup_pop(0); // reinsert_state_new
		if(result) {
			if(process_created) {
				if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
					report_error_pthread_rwlock_rdlock(status);
					exit_sigint();
				}
				pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);
					// TODO: Revisar el e_Exit_Reason y el flujo a ver si es correcto
					if(insert_state_exit(((t_TCB **) (pcb->thread_manager.array))[0])) {
						exit_sigint();
					}
				pthread_cleanup_pop(0);
				if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
					report_error_pthread_rwlock_unlock(status);
					exit_sigint();
				}

				continue;
			}
			else {
				if(reinsert_state_new(pcb)) {
					exit_sigint();
				}
				continue;
			}
		}

		if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_rdlock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

			if(array_ready_update(((t_TCB **) (pcb->thread_manager.array))[0]->priority)) {
				exit_sigint();
			}

			if(insert_state_ready(((t_TCB **) (pcb->thread_manager.array))[0])) {
				exit_sigint();
			}

		pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
		if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_unlock(status);
			exit_sigint();
		}
	}
}

void *short_term_scheduler(void) {

	log_trace_r(&MODULE_LOGGER, "Hilo planificador de corto plazo iniciado");

	t_TCB *tcb;
	e_Eviction_Reason eviction_reason;
	int status;
	bool did_quantum_expire = false;

	while(1) {
		if(sem_wait(&SEM_SHORT_TERM_SCHEDULER)) {
			report_error_sem_wait();
			exit_sigint();
		}

		if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_rdlock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

			KILL_EXEC_TCB = false;

			if(get_state_ready(&tcb, &SEM_READY)) {
				exit_sigint();
			}

			if(tcb == NULL) {
				goto cleanup_rwlock_scheduling;
			}

			if(insert_state_exec(tcb)) {
				exit_sigint();
			}

			if((status = pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPTER))) {
				report_error_pthread_mutex_lock(status);
				exit_sigint();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_QUANTUM_INTERRUPTER);

				IS_TCB_IN_CPU = true;
				FINISH_QUANTUM = false;
				QUANTUM_EXPIRED = false;

				if(sem_post(&BINARY_QUANTUM_INTERRUPTER)) {
					report_error_sem_post();
					exit_sigint();
				}
				if(sem_wait(&BINARY_SHORT_TERM_SCHEDULER)) {
					report_error_sem_post();
					exit_sigint();
				}
				if(sem_post(&BINARY_QUANTUM_INTERRUPTER)) {
					report_error_sem_wait();
					exit_sigint();
				}

				if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, TCB_EXEC->pcb->PID, TCB_EXEC->TID, CONNECTION_CPU_DISPATCH.socket_connection.fd)) {
					log_error_r(&MODULE_LOGGER, "[%d] Error al enviar dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
					exit_sigint();
				}
				log_trace_r(&MODULE_LOGGER, "[%d] Se envía dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);

				if(receive_expected_header(THREAD_DISPATCH_HEADER, CONNECTION_CPU_DISPATCH.socket_connection.fd)) {
					log_error_r(&MODULE_LOGGER, "[%d] Error al recibir confirmación de dispatch de hilo de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
					exit_sigint();
				}
				log_trace_r(&MODULE_LOGGER, "[%d] Se recibe confirmación de dispatch de hilo de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);

			pthread_cleanup_pop(0);
			if((status = pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPTER))) {
				report_error_pthread_mutex_unlock(status);
				exit_sigint();
			}

		cleanup_rwlock_scheduling:
		pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
		if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_unlock(status);
			exit_sigint();
		}

		if(tcb == NULL) {
			continue;
		}

		SHOULD_REDISPATCH = 1;
		do {

			if(receive_thread_eviction(&eviction_reason, &(TCB_EXEC->syscall_instruction), CONNECTION_CPU_DISPATCH.socket_connection.fd)) {
				log_error_r(&MODULE_LOGGER, "[%d] Error al recibir desalojo de hilo de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
				exit_sigint();
			}
			log_trace_r(&MODULE_LOGGER, "[%d] Se recibe desalojo de hilo de [Servidor] %s [PID: %u - TID: %u - Motivo: %s]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID, EVICTION_REASON_NAMES[eviction_reason]);

			if((status = pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPTER))) {
				report_error_pthread_mutex_lock(status);
				exit_sigint();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_QUANTUM_INTERRUPTER);
				IS_TCB_IN_CPU = false;
			pthread_cleanup_pop(0);
			if((status = pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPTER))) {
				report_error_pthread_mutex_unlock(status);
				exit_sigint();
			}

			if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
				report_error_pthread_rwlock_rdlock(status);
				exit_sigint();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

			switch(eviction_reason) {
				case UNEXPECTED_ERROR_EVICTION_REASON:
					TCB_EXEC->exit_reason = UNEXPECTED_ERROR_EXIT_REASON;
					if(get_state_exec(&TCB_EXEC)) {
						exit_sigint();
					}
					if(insert_state_exit(TCB_EXEC)) {
						exit_sigint();
					}
					SHOULD_REDISPATCH = 0;
					break;

				case SEGMENTATION_FAULT_EVICTION_REASON:
					TCB_EXEC->exit_reason = SEGMENTATION_FAULT_EXIT_REASON;
					if(get_state_exec(&TCB_EXEC)) {
						exit_sigint();
					}
					if(insert_state_exit(TCB_EXEC)) {
						exit_sigint();
					}
					SHOULD_REDISPATCH = 0;
					break;

				case EXIT_EVICTION_REASON:
					if(syscall_execute(&(TCB_EXEC->syscall_instruction))) {
						// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC)
						if(get_state_exec(&TCB_EXEC)) {
							exit_sigint();
						}
						if(insert_state_exit(TCB_EXEC)) {
							exit_sigint();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC) y/o el SHOULD_REDISPATCH
					break;

				case KILL_EVICTION_REASON:
					TCB_EXEC->exit_reason = KILL_EXIT_REASON;
					if(get_state_exec(&TCB_EXEC)) {
						exit_sigint();
					}
					if(insert_state_exit(TCB_EXEC)) {
						exit_sigint();
					}
					SHOULD_REDISPATCH = 0;
					break;
					
				case SYSCALL_EVICTION_REASON: {

					if(KILL_EXEC_TCB) {
						TCB_EXEC->exit_reason = KILL_EXIT_REASON;
						if(get_state_exec(&TCB_EXEC)) {
							exit_sigint();
						}
						if(insert_state_exit(TCB_EXEC)) {
							exit_sigint();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					if(syscall_execute(&(TCB_EXEC->syscall_instruction))) {
						// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC)
						if(get_state_exec(&TCB_EXEC)) {
							exit_sigint();
						}
						if(insert_state_exit(TCB_EXEC)) {
							exit_sigint();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					// La syscall se encarga de settear el e_Exit_Reason (en TCB_EXEC) y/o el SHOULD_REDISPATCH
					break;
				}

				case QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON:
					log_info_r(&MINIMAL_LOGGER, "## (%u:%u) - Desalojado por fin de Quantum", TCB_EXEC->pcb->PID, TCB_EXEC->TID);

					if(KILL_EXEC_TCB) {
						TCB_EXEC->exit_reason = KILL_EXIT_REASON;
						if(get_state_exec(&TCB_EXEC)) {
							exit_sigint();
						}
						if(insert_state_exit(TCB_EXEC)) {
							exit_sigint();
						}
						SHOULD_REDISPATCH = 0;
						break;
					}

					if(get_state_exec(&TCB_EXEC)) {
						exit_sigint();
					}
					if(insert_state_ready(TCB_EXEC)) {
						exit_sigint();
					}
					SHOULD_REDISPATCH = 0;
					break;
			}

			if((status = pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPTER))) {
				report_error_pthread_mutex_lock(status);
				exit_sigint();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_QUANTUM_INTERRUPTER);

				IS_TCB_IN_CPU = true;
				did_quantum_expire = QUANTUM_EXPIRED;
	
				if(SHOULD_REDISPATCH && (!did_quantum_expire)) {

					if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, TCB_EXEC->pcb->PID, TCB_EXEC->TID, CONNECTION_CPU_DISPATCH.socket_connection.fd)) {
						log_error_r(&MODULE_LOGGER, "[%d] Error al enviar dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
						exit_sigint();
					}
					log_trace_r(&MODULE_LOGGER, "[%d] Se envía dispatch de hilo a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);

					if(receive_expected_header(THREAD_DISPATCH_HEADER, CONNECTION_CPU_DISPATCH.socket_connection.fd)) {
						log_error_r(&MODULE_LOGGER, "[%d] Error al recibir confirmación de dispatch de hilo de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);
						exit_sigint();
					}
					log_trace_r(&MODULE_LOGGER, "[%d] Se recibe confirmación de dispatch de hilo de [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_DISPATCH.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_DISPATCH.server_type], TCB_EXEC->pcb->PID, TCB_EXEC->TID);

				}

				if(!SHOULD_REDISPATCH) {
					FINISH_QUANTUM = true;
					if((status = pthread_cond_signal(&COND_QUANTUM_INTERRUPTER))) {
						report_error_pthread_cond_signal(status);
						exit_sigint();
					}
				}

			pthread_cleanup_pop(0);
			if((status = pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPTER))) {
				report_error_pthread_mutex_unlock(status);
				exit_sigint();
			}

			if(SHOULD_REDISPATCH && did_quantum_expire) {
				if(get_state_exec(&TCB_EXEC)) {
					exit_sigint();
				}
				if(insert_state_ready(TCB_EXEC)) {
					exit_sigint();
				}
				SHOULD_REDISPATCH = 0;
			}

			if(!SHOULD_REDISPATCH) {
				if(sem_post(SEM_READY)) {
					report_error_sem_post();
					exit_sigint();
				}

				if(sem_wait(&BINARY_SHORT_TERM_SCHEDULER)) {
					report_error_sem_wait();
					exit_sigint();
				}

				errno = 0;
				if(sem_trywait(SEM_READY) && (errno != EAGAIN)) {
					report_error_sem_trywait();
					exit_sigint();
				}
			}

			pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
			if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
				report_error_pthread_rwlock_unlock(status);
				exit_sigint();
			}

		} while(SHOULD_REDISPATCH);
	}

	exit_sigint();
}

void *quantum_interrupter(void) {

	log_trace_r(&MODULE_LOGGER, "Hilo de interrupciones de quantum iniciado");

	struct timespec ts_now, ts_quantum, ts_abstime;
	int interrupt, status;

	while(1) {
		if(sem_wait(&BINARY_QUANTUM_INTERRUPTER)) {
			report_error_sem_wait();
			exit_sigint();
		}

		t_Time quantum = TCB_EXEC->quantum;
		t_PID pid = TCB_EXEC->pcb->PID;
		t_TID tid = TCB_EXEC->TID;

		if(sem_post(&BINARY_SHORT_TERM_SCHEDULER)) {
			report_error_sem_post();
			exit_sigint();
		}

		if(sem_wait(&BINARY_QUANTUM_INTERRUPTER)) {
			report_error_sem_wait();
			exit_sigint();
		}

		if((quantum) == 0) {
			interrupt = 0;
			goto sem_post_binary_short_term_scheduler;
		}

		ts_quantum = timespec_from_ms(quantum);

		if((status = pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPTER))) {
			report_error_pthread_mutex_lock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_QUANTUM_INTERRUPTER);

			if(clock_gettime(CLOCK_MONOTONIC, &ts_now)) {
				report_error_clock_gettime();
				exit_sigint();
			}

			ts_abstime = timespec_add(ts_now, ts_quantum);

			while((!FINISH_QUANTUM) && (status == 0)) {
				status = pthread_cond_timedwait(&COND_QUANTUM_INTERRUPTER, &MUTEX_QUANTUM_INTERRUPTER, &ts_abstime);
			}

			switch(status) {
				case 0:
					interrupt = 0;
					log_trace_r(&MODULE_LOGGER, "(%u:%u): Le quedó quantum remanente", pid, tid);
					break;
				case ETIMEDOUT:
					interrupt = 1;
					log_trace_r(&MODULE_LOGGER, "(%u:%u): Se expiró su quantum de %li ms", pid, tid, quantum);
					break;
				default:
					report_error_pthread_cond_timedwait(status);
					exit_sigint();
			}

		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPTER))) {
			report_error_pthread_mutex_unlock(status);
			exit_sigint();
		}

		if(!interrupt) {
			goto sem_post_binary_short_term_scheduler;
		}

		if(sem_wait(SEM_READY)) {
			report_error_sem_wait();
			exit_sigint();
		}

		if(sem_post(SEM_READY)) {
			report_error_sem_post();
			exit_sigint();
		}

		if((status = pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPTER))) {
			report_error_pthread_mutex_lock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_QUANTUM_INTERRUPTER);

			QUANTUM_EXPIRED = true;

			if(IS_TCB_IN_CPU) {
				if(send_kernel_interrupt(QUANTUM_KERNEL_INTERRUPT, pid, tid, CONNECTION_CPU_INTERRUPT.socket_connection.fd)) {
					log_error_r(&MODULE_LOGGER, "[%d] Error al enviar interrupción a [Servidor] %s [PID: %u - TID: %u - Interrupción: %s]", CONNECTION_CPU_INTERRUPT.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_INTERRUPT.server_type], pid, tid, KERNEL_INTERRUPT_NAMES[QUANTUM_KERNEL_INTERRUPT]);
					exit_sigint();
				}
				log_trace_r(&MODULE_LOGGER, "[%d] Se envía interrupción a [Servidor] %s [PID: %u - TID: %u - Interrupción: %s]", CONNECTION_CPU_INTERRUPT.socket_connection.fd, PORT_NAMES[CONNECTION_CPU_INTERRUPT.server_type], pid, tid, KERNEL_INTERRUPT_NAMES[QUANTUM_KERNEL_INTERRUPT]);
			}

		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPTER))) {
			report_error_pthread_mutex_unlock(status);
			exit_sigint();
		}

		sem_post_binary_short_term_scheduler:
		if(sem_post(&BINARY_SHORT_TERM_SCHEDULER)) {
			report_error_sem_post();
			exit_sigint();
		}
	}

	return NULL;
}

void *io_device(void) {

	log_trace_r(&MODULE_LOGGER, "Hilo de IO iniciado");

	t_TCB *tcb;
	t_Time time;
	struct timespec ts_now, ts_time, ts_abstime;
	int status;

	while(1) {
		if(sem_wait(&SEM_IO_DEVICE)) {
			report_error_sem_wait();
			exit_sigint();
		}

		if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_rdlock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

			if(get_state_blocked_io_ready(&tcb)) {
				exit_sigint();
			}

			if(tcb == NULL) {
				goto cleanup_rwlock_scheduling;
			}

			if(insert_state_blocked_io_exec(tcb)) {
				exit_sigint();
			}

			payload_remove(&(TCB_BLOCKED_IO_EXEC->syscall_instruction), &time, sizeof(time));

			CANCEL_IO_OPERATION = false;

		cleanup_rwlock_scheduling:
		pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
		if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_unlock(status);
			exit_sigint();
		}

		if(tcb == NULL) {
			continue;
		}

		ts_time = timespec_from_ms(time);

		if((status = pthread_mutex_lock(&MUTEX_CANCEL_IO_OPERATION))) {
			report_error_pthread_mutex_lock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_CANCEL_IO_OPERATION);

			if(clock_gettime(CLOCK_MONOTONIC, &ts_now)) {
				report_error_clock_gettime();
				exit_sigint();
			}

			ts_abstime = timespec_add(ts_now, ts_time);

			while((!CANCEL_IO_OPERATION) && (status == 0)) {
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
					report_error_pthread_cond_timedwait(status);
					exit_sigint();
			}

		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&MUTEX_CANCEL_IO_OPERATION))) {
			report_error_pthread_mutex_unlock(status);
			exit_sigint();
		}


		if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_rdlock(status);
			exit_sigint();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

			if(get_state_blocked_io_exec(&tcb)) {
				exit_sigint();
			}

			if(tcb != NULL) {
				log_info_r(&MINIMAL_LOGGER, "## (%u:%u) finalizó IO y pasa a READY", tcb->pcb->PID, tcb->TID);
				if(insert_state_ready(tcb)) {
					exit_sigint();
				}
			}

		pthread_cleanup_pop(0);
		if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
			report_error_pthread_rwlock_unlock(status);
			exit_sigint();
		}
	}

	exit_sigint();
}

int wait_free_memory(void) {
	int retval = 0, status;

	if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
		report_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_FREE_MEMORY);
		while(!FREE_MEMORY) {
			if((status = pthread_cond_wait(&COND_FREE_MEMORY, &MUTEX_FREE_MEMORY))) {
				report_error_pthread_cond_wait(status);
				retval = -1;
				break;
			}
		}
	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
		report_error_pthread_mutex_unlock(status);
		retval = -1;
		goto ret;
	}

	ret:
	return retval;
}

void *dump_memory_petitioner(t_Dump_Memory_Petition *dump_memory_petition) {

	int result = 0;

	dump_memory_petition->result = &result;

	pthread_cleanup_push((void (*)(void *)) remove_dump_memory_thread, dump_memory_petition);

	log_trace_r(&MODULE_LOGGER, "Hilo de petición de volcado de memoria iniciado");

	t_Connection connection_memory = CONNECTION_MEMORY_INITIALIZER;

	client_thread_connect_to_server(&connection_memory);
	pthread_cleanup_push((void (*)(void *)) wrapper_close, &(connection_memory.socket_connection.fd));

		if(send_pid_and_tid_with_header(DUMP_MEMORY_HEADER, dump_memory_petition->pid, dump_memory_petition->tid, connection_memory.socket_connection.fd)) {
			log_error_r(&MODULE_LOGGER, "[%d] Error al enviar solicitud de volcado de memoria a [Servidor] %s [PID: %u - TID: %u]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], dump_memory_petition->pid, dump_memory_petition->tid);
			exit_sigint();
		}
		log_trace_r(&MODULE_LOGGER, "[%d] Se envía solicitud de volcado de memoria a [Servidor] %s [PID: %u - TID: %u]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], dump_memory_petition->pid, dump_memory_petition->tid);

		if(receive_result_with_expected_header(DUMP_MEMORY_HEADER, &result, connection_memory.socket_connection.fd)) {
			log_error_r(&MODULE_LOGGER, "[%d] Error al recibir resultado de volcado de memoria de [Servidor] %s [PID: %u - TID: %u]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], dump_memory_petition->pid, dump_memory_petition->tid);
			exit_sigint();
		}
		log_trace_r(&MODULE_LOGGER, "[%d] Se recibe resultado de volcado de memoria de [Servidor] %s [PID: %u - TID: %u - Resultado: %d]", connection_memory.socket_connection.fd, PORT_NAMES[connection_memory.server_type], dump_memory_petition->pid, dump_memory_petition->tid, result);

	pthread_cleanup_pop(0);
	if(close(connection_memory.socket_connection.fd)) {
		report_error_close();
		exit_sigint();
	}

	pthread_cleanup_pop(0);
	if(remove_dump_memory_thread(dump_memory_petition)) {
		exit_sigint();
	}

	pthread_exit(NULL);
}

int remove_dump_memory_thread(t_Dump_Memory_Petition *dump_memory_petition) {
	int retval = 0, status;

	pthread_cleanup_push((void (*)(void *)) free, dump_memory_petition);

		if((*(dump_memory_petition->result)) == 0) {

			if((status = pthread_rwlock_rdlock(&RWLOCK_SCHEDULING))) {
				report_error_pthread_rwlock_rdlock(status);
				retval = -1;
				goto cleanup_dump_memory_petition;
			}
			pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

				if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex)))) {
					report_error_pthread_mutex_lock(status);
					retval = -1;
					goto cleanup_rwlock_rdlock_scheduling;
				}
				pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex));

					list_remove_by_condition_with_comparation(SHARED_LIST_BLOCKED_DUMP_MEMORY.list, (bool (*)(void *, void *)) pointers_match, dump_memory_petition);

					if((SHARED_LIST_BLOCKED_DUMP_MEMORY.list->head) == NULL) {
						if((status = pthread_cond_signal(&COND_BLOCKED_DUMP_MEMORY))) {
							report_error_pthread_cond_signal(status);
							retval = -1;
						}
					}

				pthread_cleanup_pop(0); // SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex
				if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_DUMP_MEMORY.mutex)))) {
					report_error_pthread_mutex_unlock(status);
					retval = -1;
					goto cleanup_rwlock_rdlock_scheduling;
				}

				if((retval) || ((dump_memory_petition->tcb) == NULL)) {
					goto cleanup_rwlock_rdlock_scheduling;
				}


				if(insert_state_ready(dump_memory_petition->tcb)) {
					retval = -1;
					goto cleanup_rwlock_rdlock_scheduling;
				}

			cleanup_rwlock_rdlock_scheduling:
			pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
			if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
				report_error_pthread_rwlock_unlock(status);
				retval = -1;
				goto cleanup_dump_memory_petition;
			}

		}
		else {

			if((status = pthread_rwlock_wrlock(&RWLOCK_SCHEDULING))) {
				report_error_pthread_rwlock_wrlock(status);
				retval = -1;
				goto cleanup_dump_memory_petition;
			}
			pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &RWLOCK_SCHEDULING);

				if((dump_memory_petition->tcb) != NULL) {
					dump_memory_petition->tcb->exit_reason = DUMP_MEMORY_ERROR_EXIT_REASON;
					if(insert_state_exit(dump_memory_petition->tcb)) {
						retval = -1;
						goto cleanup_rwlock_wrlock_scheduling;
					}
					if(kill_process(dump_memory_petition->tcb->pcb, DUMP_MEMORY_ERROR_EXIT_REASON)) {
						retval = -1;
						goto cleanup_rwlock_wrlock_scheduling;
					}
				}

				list_remove_by_condition_with_comparation(SHARED_LIST_BLOCKED_DUMP_MEMORY.list, (bool (*)(void *, void *)) pointers_match, dump_memory_petition);

				if((SHARED_LIST_BLOCKED_DUMP_MEMORY.list->head) == NULL) {
					if((status = pthread_cond_signal(&COND_BLOCKED_DUMP_MEMORY))) {
						report_error_pthread_cond_signal(status);
						retval = -1;
					}
				}

			cleanup_rwlock_wrlock_scheduling:
			pthread_cleanup_pop(0); // RWLOCK_SCHEDULING
			if((status = pthread_rwlock_unlock(&RWLOCK_SCHEDULING))) {
				report_error_pthread_rwlock_unlock(status);
				retval = -1;
				goto cleanup_dump_memory_petition;
			}

		}

	cleanup_dump_memory_petition:
	pthread_cleanup_pop(1); // dump_memory_petition

	return retval;
}

bool dump_memory_petition_matches_tcb(t_Dump_Memory_Petition *dump_memory_petition, t_TCB *tcb) {
    return dump_memory_petition->tcb == tcb;
}

int signal_free_memory(void) {
	int retval = 0, status;

	if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
		report_error_pthread_mutex_lock(status);
		retval = -1;
		goto ret;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_FREE_MEMORY);
		FREE_MEMORY = 1;
		if((status = pthread_cond_signal(&COND_FREE_MEMORY))) {
			report_error_pthread_cond_signal(status);
			retval = -1;
		}
	pthread_cleanup_pop(0);
	if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
		report_error_pthread_mutex_unlock(status);
		retval = -1;
		goto ret;
	}

	ret:
	return retval;
}