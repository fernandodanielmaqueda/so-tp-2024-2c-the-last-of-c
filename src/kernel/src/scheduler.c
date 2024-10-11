#include "scheduler.h"

t_Shared_List SHARED_LIST_NEW = { .list = NULL };

t_Drain_Ongoing_Resource_Sync READY_SYNC = { .initialized = false };
t_Shared_List *ARRAY_LIST_READY = NULL;
t_Priority PRIORITY_COUNT = 0;

t_Shared_List SHARED_LIST_EXEC = { .list = NULL };

t_Shared_List SHARED_LIST_BLOCKED_MEMORY_DUMP = { .list = NULL };

t_Shared_List SHARED_LIST_BLOCKED_IO_READY = { .list = NULL };
t_Shared_List SHARED_LIST_BLOCKED_IO_EXEC = { .list = NULL };

t_Shared_List SHARED_LIST_EXIT = { .list = NULL };

t_PThread_Controller THREAD_LONG_TERM_SCHEDULER_NEW = { .was_created = false };
sem_t SEM_LONG_TERM_SCHEDULER_NEW;

t_PThread_Controller THREAD_LONG_TERM_SCHEDULER_EXIT = { .was_created = false };
sem_t SEM_LONG_TERM_SCHEDULER_EXIT;

t_Time QUANTUM;
t_PThread_Controller THREAD_QUANTUM_INTERRUPTER = { .was_created = false };
sem_t BINARY_QUANTUM;
bool QUANTUM_INTERRUPT;
pthread_mutex_t MUTEX_QUANTUM_INTERRUPT;

sem_t SEM_SHORT_TERM_SCHEDULER;
bool IS_TCB_IN_CPU = false;
pthread_mutex_t MUTEX_IS_TCB_IN_CPU;

bool FREE_MEMORY = 1;
pthread_mutex_t MUTEX_FREE_MEMORY;
pthread_cond_t COND_FREE_MEMORY;

bool KILL_EXEC_PROCESS;
pthread_mutex_t MUTEX_KILL_EXEC_PROCESS;

t_TCB *EXEC_TCB = NULL;

bool SHOULD_REDISPATCH = 0;

t_Drain_Ongoing_Resource_Sync SCHEDULING_SYNC;

int initialize_long_term_scheduler(void) {
	int status;

	if(create_pthread(&THREAD_LONG_TERM_SCHEDULER_NEW, (void *(*)(void *)) long_term_scheduler_new, NULL)) {
		goto error;
	}

	if(create_pthread(&THREAD_LONG_TERM_SCHEDULER_EXIT, (void *(*)(void *)) long_term_scheduler_exit, NULL)) {
		goto error_thread_long_term_scheduler_new;
	}

	return 0;

	error_thread_long_term_scheduler_new:
		cancel_pthread(&THREAD_LONG_TERM_SCHEDULER_NEW);
	error:
		return -1;
}

int finish_long_term_scheduler(void) {
	int retval = 0, status;

	if(cancel_pthread(&THREAD_LONG_TERM_SCHEDULER_NEW)) {
		retval = -1;
	}

	if(cancel_pthread(&THREAD_LONG_TERM_SCHEDULER_EXIT)) {
		retval = -1;
	}

	return retval;
}

void *long_term_scheduler_new(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en NEW) iniciado");

	t_Connection connection_memory = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};
	t_PCB *pcb;
	int status;

	while(1) {
		if(wait_free_memory()) {
			error_pthread();
		}
	
		if(sem_wait(&SEM_LONG_TERM_SCHEDULER_NEW)) {
			log_error_sem_wait();
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) sem_post, &SEM_LONG_TERM_SCHEDULER_NEW);

		if(wait_draining_requests(&SCHEDULING_SYNC)) {
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) signal_draining_requests, &SCHEDULING_SYNC);

			if(((status = pthread_mutex_lock(&(SHARED_LIST_NEW.mutex))))) {
				log_error_pthread_mutex_lock(status);
				error_pthread();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_NEW.mutex));

				if((SHARED_LIST_NEW.list)->head == NULL) {
					status = -1; // Empty
				}
				else {
					pcb = (t_PCB *) (SHARED_LIST_NEW.list)->head->data;
				}

			pthread_cleanup_pop(1);

			if(status) {
				goto cleanup;
			}

			client_thread_connect_to_server(&connection_memory);

				if(send_process_create(pcb->PID, pcb->size, connection_memory.fd_connection)) {
					// TODO
				}

				int result;
				if(receive_result_with_expected_header(PROCESS_CREATE_HEADER, &result, connection_memory.fd_connection)) {
					// TODO
				}

				if(result) {
					if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
						log_error_pthread_mutex_lock(status);
						// TODO
					}
						FREE_MEMORY = 0;
					if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
						log_error_pthread_mutex_unlock(status);
						// TODO
					}

					if(close(connection_memory.fd_connection)) {
						log_error_close();
						// TODO
					}
					signal_draining_requests(&SCHEDULING_SYNC);
					if(sem_post(&SEM_LONG_TERM_SCHEDULER_NEW)) {
						log_error_sem_post();
						// TODO
					}

					continue;
				}

			if(close(connection_memory.fd_connection)) {
				log_error_close();
				// TODO
			}

			client_thread_connect_to_server(&connection_memory);

				if(send_thread_create(pcb->PID, (t_TID) 0, ((t_TCB **) (pcb->thread_manager.cb_array))[0]->pseudocode_filename, connection_memory.fd_connection)) {
					// TODO
				}

				if(receive_expected_header(THREAD_CREATE_HEADER, connection_memory.fd_connection)) {
					// TODO
				}

			if(close(connection_memory.fd_connection)) {
				log_error_close();
				// TODO
			}

			switch(SCHEDULING_ALGORITHM) {

				case FIFO_SCHEDULING_ALGORITHM:
					break;

				case PRIORITIES_SCHEDULING_ALGORITHM:
				case MLQ_SCHEDULING_ALGORITHM:
					if(request_ready_list(((t_TCB **) (pcb->thread_manager.cb_array))[0]->priority)) {
					}
					break;
			}

			switch_process_state(((t_TCB **) (pcb->thread_manager.cb_array))[0], READY_STATE);

	cleanup:
		pthread_cleanup_pop(1);
		pthread_cleanup_pop(status);
	}
}

void *long_term_scheduler_exit(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en EXIT) iniciado");

	t_TCB *tcb;
	int status;

	while(1) {
		if(sem_wait(&SEM_LONG_TERM_SCHEDULER_EXIT)) {
			log_error_sem_wait();
			// TODO
		}

		wait_draining_requests(&SCHEDULING_SYNC);
			if((status = pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex)))) {
				log_error_pthread_mutex_lock(status);
				// TODO
			}
				tcb = (t_TCB *) list_remove((SHARED_LIST_EXIT.list), 0);
			if((status = pthread_mutex_unlock(&(SHARED_LIST_EXIT.mutex)))) {
				log_error_pthread_mutex_unlock(status);
				// TODO
			}
		signal_draining_requests(&SCHEDULING_SYNC);

		// Libera recursos asignados al proceso
		/*
		while(pcb->assigned_resources->head != NULL) {

			t_Resource *resource = (t_Resource *) list_remove(pcb->assigned_resources, 0);

			if(status = pthread_mutex_lock(&(resource->mutex_instances))) {
				log_error_pthread_mutex_lock(status);
				// TODO
			}

			resource->instances++;
				
			if(resource->instances <= 0) {
				if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
					log_error_pthread_mutex_unlock(status);
					// TODO
				}

				if(status = pthread_mutex_lock(&(resource->shared_list_blocked.mutex))) {
					log_error_pthread_mutex_lock(status);
					// TODO
				}

					if((resource->shared_list_blocked.list)->head == NULL) {
						if(status = pthread_mutex_unlock(&(resource->shared_list_blocked.mutex))) {
							log_error_pthread_mutex_unlock(status);
							// TODO
						}
						continue;
					}

					t_PCB *pcb = (t_PCB *) list_remove(resource->shared_list_blocked.list, 0);

				if(status = pthread_mutex_unlock(&(resource->shared_list_blocked.mutex))) {
					log_error_pthread_mutex_unlock(status);
					// TODO
				}

				list_add(pcb->assigned_resources, resource);

				switch_process_state(pcb, READY_STATE);
			}
			else {
				if(status = pthread_mutex_unlock(&(resource->mutex_instances))) {
					log_error_pthread_mutex_unlock(status);
					// TODO
				}
			}
		}

		if(send_process_destroy(pcb->PID, CONNECTION_MEMORY.fd_connection)) {
			// TODO
			exit(EXIT_FAILURE);
		}

		if(receive_expected_header(PROCESS_DESTROY_HEADER, CONNECTION_MEMORY.fd_connection)) {
			// TODO
        	exit(EXIT_FAILURE);
		}
		*/

		//log_info(MINIMAL_LOGGER, "Finaliza el proceso <%d> - Motivo: <%s>", pcb->PID, EXIT_REASONS[pcb->exit_reason]);

		tid_release(&(tcb->pcb->thread_manager), tcb->TID);
		tcb_destroy(tcb);
	}

	return NULL;
}

void sigusr1_quantum_handler(int signal) {
	// No hace nada; sólo la utilizo para despertarme del usleep
}

void *quantum_interrupter(void *NULL_parameter) {
	int status;

	sigset_t set_SIGUSR1, set_SIGINT;

	if(sigemptyset(&set_SIGUSR1)) {
		log_error_sigemptyset();
		error_pthread();
	}
	if(sigemptyset(&set_SIGINT)) {
		log_error_sigemptyset();
		error_pthread();
	}

	if(sigaddset(&set_SIGUSR1, SIGUSR1)) {
		log_error_sigaddset();
		error_pthread();
	}
	if(sigaddset(&set_SIGINT, SIGINT)) {
		log_error_sigaddset();
		error_pthread();
	}

	if((status = pthread_sigmask(SIG_BLOCK, &set_SIGUSR1, NULL))) {
		log_error_pthread_sigmask(status);
		error_pthread();
	}
	if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
		log_error_pthread_sigmask(status);
		error_pthread();
	}

	if(sigaction(SIGUSR1, &(struct sigaction) {.sa_handler = sigusr1_quantum_handler}, NULL)) {
		log_error_sigaction();	
		error_pthread();
	}

	log_trace(MODULE_LOGGER, "Hilo de interrupciones de CPU iniciado");

	while(1) {
		if(sem_wait(&BINARY_QUANTUM)) {
			log_error_sem_wait();
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) sem_post, &BINARY_QUANTUM);

		if((status = pthread_sigmask(SIG_UNBLOCK, &set_SIGINT, NULL))) {
			log_error_pthread_sigmask(status);
			error_pthread();
		}

			usleep(EXEC_TCB->quantum * 1000);

		if((status = pthread_sigmask(SIG_BLOCK, &set_SIGINT, NULL))) {
			log_error_pthread_sigmask(status);
			error_pthread();
		}

		if((status = pthread_mutex_lock(&MUTEX_IS_TCB_IN_CPU))) {
			log_error_pthread_mutex_lock(status);
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &MUTEX_IS_TCB_IN_CPU);
			if(!IS_TCB_IN_CPU) {
				status = -1;
			}
		pthread_cleanup_pop(1);

		if(status) {
			goto cleanup;
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

		if((status = pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPT))) {
			log_error_pthread_mutex_lock(status);
			// TODO
		}
			QUANTUM_INTERRUPT = 1;
		if((status = pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPT))) {
			log_error_pthread_mutex_unlock(status);
			// TODO
		}

		if(send_kernel_interrupt(QUANTUM_KERNEL_INTERRUPT, EXEC_TCB->pcb->PID, EXEC_TCB->TID, CONNECTION_CPU_INTERRUPT.fd_connection)) {
			// TODO
			exit(EXIT_FAILURE);
		}

		log_trace(MODULE_LOGGER, "(%u:%u) - Se envia interrupcion por quantum tras %li milisegundos", EXEC_TCB->pcb->PID, EXEC_TCB->TID, EXEC_TCB->quantum);
	
		cleanup:
			pthread_cleanup_pop(status);
	}

	return NULL;
}

void *short_term_scheduler(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de corto plazo iniciado");
 
	e_Eviction_Reason eviction_reason;
	t_Payload syscall_instruction;
	int status;

	while(1) {
		if(sem_wait(&SEM_SHORT_TERM_SCHEDULER)) {
			log_error_sem_wait();
			// TODO
		}

		wait_draining_requests(&SCHEDULING_SYNC);

			EXEC_TCB = NULL;
			switch(SCHEDULING_ALGORITHM) {

				case FIFO_SCHEDULING_ALGORITHM:

					if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[0].mutex)))) {
						log_error_pthread_mutex_lock(status);
						// TODO
					}
						if((ARRAY_LIST_READY[0].list)->head != NULL)
							EXEC_TCB = (t_TCB *) list_remove((ARRAY_LIST_READY[0].list), 0);
					if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[0].mutex)))) {
						log_error_pthread_mutex_unlock(status);
						// TODO
					}

					break;
				
				case PRIORITIES_SCHEDULING_ALGORITHM:
				case MLQ_SCHEDULING_ALGORITHM:
					
					for(register t_Priority priority = 0; priority < PRIORITY_COUNT; priority++) {
						if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[priority].mutex)))) {
							log_error_pthread_mutex_lock(status);
							// TODO
						}
							if((ARRAY_LIST_READY[priority].list)->head != NULL) {
								EXEC_TCB = (t_TCB *) list_remove((ARRAY_LIST_READY[priority].list), 0);
								if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[priority].mutex)))) {
									log_error_pthread_mutex_unlock(status);
									// TODO
								}
								break;
							}
						if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[priority].mutex)))) {
							log_error_pthread_mutex_unlock(status);
							// TODO
						}
					}

					break;

			}

			if(EXEC_TCB == NULL) {
				signal_draining_requests(&SCHEDULING_SYNC);
				continue;
			}

			switch_process_state(EXEC_TCB, EXEC_STATE);

			SHOULD_REDISPATCH = 1;
			while(SHOULD_REDISPATCH) {

				if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, EXEC_TCB->pcb->PID, EXEC_TCB->TID, CONNECTION_CPU_DISPATCH.fd_connection)) {
					// TODO
					exit(EXIT_FAILURE);
				}

				signal_draining_requests(&SCHEDULING_SYNC);

				IS_TCB_IN_CPU = true;

				switch(SCHEDULING_ALGORITHM) {

					case FIFO_SCHEDULING_ALGORITHM:
					case PRIORITIES_SCHEDULING_ALGORITHM:
						break;

					case MLQ_SCHEDULING_ALGORITHM:
						QUANTUM_INTERRUPT = 0;
						if(sem_post(&BINARY_QUANTUM)) {
							log_error_sem_post();
							error_pthread();
						}
						break;

				}

				if(receive_thread_eviction(&eviction_reason, &syscall_instruction, CONNECTION_CPU_DISPATCH.fd_connection)) {
					// TODO
					exit(EXIT_FAILURE);
				}

				IS_TCB_IN_CPU = false;

				switch(SCHEDULING_ALGORITHM) {

					case FIFO_SCHEDULING_ALGORITHM:
					case PRIORITIES_SCHEDULING_ALGORITHM:
						break;

					case MLQ_SCHEDULING_ALGORITHM:
						pthread_kill(THREAD_QUANTUM_INTERRUPTER.thread, SIGUSR1);

						if((status = pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPT))) {
							log_error_pthread_mutex_lock(status);
							// TODO
						}
							if(!QUANTUM_INTERRUPT)
								pthread_kill(THREAD_QUANTUM_INTERRUPTER.thread, SIGUSR1);
								/*
								if((status = pthread_cancel(THREAD_QUANTUM_INTERRUPT))) {
									log_error_pthread_cancel(status);
									// TODO
								}
								*/
						if((status = pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPT))) {
							log_error_pthread_mutex_unlock(status);
							// TODO
						}
						/*
						if((status = pthread_join(THREAD_QUANTUM_INTERRUPT, NULL))) {
							log_error_pthread_join(status);
							// TODO
						}
						*/

						break;

				}

				wait_draining_requests(&SCHEDULING_SYNC);

				switch(eviction_reason) {
					case UNEXPECTED_ERROR_EVICTION_REASON:
						switch_process_state(EXEC_TCB, EXIT_STATE);
						SHOULD_REDISPATCH = 0;
						break;

					case SEGMENTATION_FAULT_EVICTION_REASON:
						switch_process_state(EXEC_TCB, EXIT_STATE);
						SHOULD_REDISPATCH = 0;
						break;

					case EXIT_EVICTION_REASON:
						switch_process_state(EXEC_TCB, EXIT_STATE);
						SHOULD_REDISPATCH = 0;
						break;

					case KILL_KERNEL_INTERRUPT_EVICTION_REASON:
						/*
						if(status = pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS)) {
							log_error_pthread_mutex_lock(status);
							// TODO
						}
							KILL_EXEC_PROCESS = 0;
						if(status = pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS)) {
							log_error_pthread_mutex_unlock(status);
							// TODO
						}
						*/
						switch_process_state(EXEC_TCB, EXIT_STATE);
						SHOULD_REDISPATCH = 0;
						break;
						
					case SYSCALL_EVICTION_REASON:
						/*
						if(status = pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS)) {
							log_error_pthread_mutex_lock(status);
							// TODO
						}
							if(KILL_EXEC_PROCESS) {
								KILL_EXEC_PROCESS = 0;
								if(status = pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS)) {
									log_error_pthread_mutex_unlock(status);
									// TODO
								}
								switch_process_state(EXEC_TCB, EXIT_STATE);
								SHOULD_REDISPATCH = 0;
								break;
							}
						if(status = pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS)) {
							log_error_pthread_mutex_unlock(status);
							// TODO
						}
						*/

						status = syscall_execute(&syscall_instruction);

						if(status) {
							// La syscall se encarga de settear el e_Exit_Reason (en EXEC_TCB)
							switch_process_state(EXEC_TCB, EXIT_STATE);
							SHOULD_REDISPATCH = 0;
							break;
						}

						// La syscall se encarga de settear el e_Exit_Reason (en EXEC_TCB) y/o el SHOULD_REDISPATCH
						break;

					case QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON:
						log_info(MINIMAL_LOGGER, "## (%u:%u) - Desalojado por fin de Quantum", EXEC_TCB->pcb->PID, EXEC_TCB->TID);

						/*
						if(status = pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS)) {
							log_error_pthread_mutex_lock(status);
							// TODO
						}
							if(KILL_EXEC_PROCESS) {
								KILL_EXEC_PROCESS = 0;
								if(status = pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS)) {
									log_error_pthread_mutex_unlock(status);
									// TODO
								}
								EXEC_TCB->exit_reason = INTERRUPTED_BY_USER_EXIT_REASON;
								switch_process_state(EXEC_TCB, EXIT_STATE);
								SHOULD_REDISPATCH = 0;
								break;
							}
						if(status = pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS)) {
							log_error_pthread_mutex_unlock(status);
							// TODO
						}
						*/

						switch_process_state(EXEC_TCB, READY_STATE);
						SHOULD_REDISPATCH = 0;
						break;
				}
			}
		signal_draining_requests(&SCHEDULING_SYNC);
	}

	return NULL;
}

int wait_free_memory(void) {
	int status;

	if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_lock(status);
		// TODO
	}
		while(!FREE_MEMORY) {
			if((status = pthread_cond_wait(&COND_FREE_MEMORY, &MUTEX_FREE_MEMORY))) {
				log_error_pthread_cond_wait(status);
				// TODO
			}
		}
	if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_unlock(status);
		// TODO
	}

	return 0;
}

int signal_free_memory(void) {
	int status;
	
	if((status = pthread_mutex_lock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_lock(status);
		// TODO
	}
		FREE_MEMORY = 1;
		if((status = pthread_cond_signal(&COND_FREE_MEMORY))) {
			log_error_pthread_cond_signal(status);
			// TODO
		}
	if((status = pthread_mutex_unlock(&MUTEX_FREE_MEMORY))) {
		log_error_pthread_mutex_unlock(status);
		// TODO
	}

	return 0;
}

void switch_process_state(t_TCB *tcb, e_Process_State new_state) {

	e_Process_State previous_state = tcb->current_state;
	int status;

	switch(previous_state) {

		case READY_STATE:
		{
			t_Shared_List *shared_list_state = tcb->shared_list_state;
			wait_draining_requests(&READY_SYNC);
				if((status = pthread_mutex_lock(&(shared_list_state->mutex)))) {
					log_error_pthread_mutex_lock(status);
					// TODO
				}
					list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) tcb_matches_tid, &(tcb->TID));
					tcb->shared_list_state = NULL;
				if((status = pthread_mutex_unlock(&(shared_list_state->mutex)))) {
					log_error_pthread_mutex_unlock(status);
					// TODO
				}
			signal_draining_requests(&READY_SYNC);
			break;
		}

		case EXEC_STATE:
		{
			if((status = pthread_mutex_lock(&(SHARED_LIST_EXEC.mutex)))) {
				log_error_pthread_mutex_lock(status);
				// TODO
			}
				list_remove_by_condition_with_comparation((SHARED_LIST_EXEC.list), (bool (*)(void *, void *)) tcb_matches_tid, &(tcb->TID));
				tcb->shared_list_state = NULL;
			if((status = pthread_mutex_unlock(&(SHARED_LIST_EXEC.mutex)))) {
				log_error_pthread_mutex_unlock(status);
				// TODO
			}
			break;
		}

		default:
			break;
	}

	tcb->current_state = new_state;

	switch(new_state) {

		case READY_STATE:
		{

			switch(SCHEDULING_ALGORITHM) {

				case FIFO_SCHEDULING_ALGORITHM:
					if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[0].mutex)))) {
						log_error_pthread_mutex_lock(status);
						// TODO
					}
						list_add((ARRAY_LIST_READY[0].list), tcb);
						tcb->shared_list_state = &(ARRAY_LIST_READY[0]);
					if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[0].mutex)))) {
						log_error_pthread_mutex_unlock(status);
						// TODO
					}
					break;
				
				case PRIORITIES_SCHEDULING_ALGORITHM:
				case MLQ_SCHEDULING_ALGORITHM:
					if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[tcb->priority].mutex)))) {
						log_error_pthread_mutex_lock(status);
						// TODO
					}
						list_add((ARRAY_LIST_READY[tcb->priority].list), tcb);
						tcb->shared_list_state = &(ARRAY_LIST_READY[tcb->priority]);
					if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[tcb->priority].mutex)))) {
						log_error_pthread_mutex_unlock(status);
						// TODO
					}
					break;
			}

			if(sem_post(&SEM_SHORT_TERM_SCHEDULER)) {
				log_error_sem_post();
				// TODO
			}

			break;
		}

		case EXEC_STATE:
		{
			if((status = pthread_mutex_lock(&(SHARED_LIST_EXEC.mutex)))) {
				log_error_pthread_mutex_lock(status);
				// TODO
			}
				log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <EXEC>", (int) tcb->TID, STATE_NAMES[previous_state]);
				list_add((SHARED_LIST_EXEC.list), tcb);
				tcb->shared_list_state = &(SHARED_LIST_EXEC);
			if((status = pthread_mutex_unlock(&(SHARED_LIST_EXEC.mutex)))) {
				log_error_pthread_mutex_unlock(status);
				// TODO
			}
			break;
		}

		case EXIT_STATE:
		{
			if((status = pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex)))) {
				log_error_pthread_mutex_lock(status);
				// TODO
			}
				list_add((SHARED_LIST_EXIT.list), tcb);
				tcb->shared_list_state = &(SHARED_LIST_EXIT);
			if((status = pthread_mutex_unlock(&(SHARED_LIST_EXIT.mutex)))) {
				log_error_pthread_mutex_unlock(status);
				// TODO
			}

			if(sem_post(&SEM_LONG_TERM_SCHEDULER_EXIT)) {
				log_error_sem_post();
				// TODO
			}

			break;
		}

		default:
			break;
	}
}