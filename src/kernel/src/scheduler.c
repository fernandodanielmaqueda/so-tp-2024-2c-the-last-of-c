#include "scheduler.h"

t_Shared_List SHARED_LIST_NEW;

t_Priority PRIORITY_COUNT = 0;
t_Shared_List **ARRAY_LIST_READY = NULL;
t_Drain_Ongoing_Resource_Sync READY_SYNC;

t_Shared_List SHARED_LIST_EXEC;
t_Shared_List SHARED_LIST_EXIT;

pthread_t THREAD_LONG_TERM_SCHEDULER_NEW;
sem_t SEM_LONG_TERM_SCHEDULER_NEW;

pthread_t THREAD_LONG_TERM_SCHEDULER_EXIT;
sem_t SEM_LONG_TERM_SCHEDULER_EXIT;

pthread_t THREAD_CPU_INTERRUPTER;

sem_t SEM_SHORT_TERM_SCHEDULER;

int EXEC_TCB;

char *SCHEDULING_ALGORITHMS[] = {
	[FIFO_SCHEDULING_ALGORITHM] = "FIFO",
	[PRIORITIES_SCHEDULING_ALGORITHM] = "PRIORIDADES",
	[MLQ_SCHEDULING_ALGORITHM] = "CMN"
};

e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

t_Quantum QUANTUM;
pthread_t THREAD_QUANTUM_INTERRUPT;
pthread_mutex_t MUTEX_QUANTUM_INTERRUPT;
int QUANTUM_INTERRUPT;

t_Drain_Ongoing_Resource_Sync SCHEDULING_SYNC;

int find_scheduling_algorithm(char *name, e_Scheduling_Algorithm *destination) {

    if(name == NULL || destination == NULL)
        return 1;
    
    size_t scheduling_algorithms_number = sizeof(SCHEDULING_ALGORITHMS) / sizeof(SCHEDULING_ALGORITHMS[0]);
    for (register e_Scheduling_Algorithm scheduling_algorithm = 0; scheduling_algorithm < scheduling_algorithms_number; scheduling_algorithm++)
        if (strcmp(SCHEDULING_ALGORITHMS[scheduling_algorithm], name) == 0) {
            *destination = scheduling_algorithm;
            return 0;
        }

    return 1;
}

void initialize_scheduling(void) {
	initialize_long_term_scheduler();
	initialize_short_term_scheduler();
}

void finish_scheduling(void) {

}

void initialize_long_term_scheduler(void) {
	pthread_create(&THREAD_LONG_TERM_SCHEDULER_NEW, NULL, (void *(*)(void *)) long_term_scheduler_new, NULL);
	pthread_detach(THREAD_LONG_TERM_SCHEDULER_NEW);

	pthread_create(&THREAD_LONG_TERM_SCHEDULER_EXIT, NULL, (void *(*)(void *)) long_term_scheduler_exit, NULL);
	pthread_detach(THREAD_LONG_TERM_SCHEDULER_EXIT);
}

void initialize_short_term_scheduler(void) {
	pthread_create(&THREAD_CPU_INTERRUPTER, NULL, (void *(*)(void *)) cpu_interrupter, NULL);
	pthread_detach(THREAD_CPU_INTERRUPTER);

	short_term_scheduler(NULL);
}

void *long_term_scheduler_new(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en NEW) iniciado");

	t_Connection connection_memory = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(MODULE_CONFIG, "IP_MEMORIA"), .port = config_get_string_value(MODULE_CONFIG, "PUERTO_MEMORIA")};
	t_PCB *pcb;

	while(1) {
		sem_wait(&SEM_LONG_TERM_SCHEDULER_NEW);
		//wait_multiprogramming_level();

		wait_draining_requests(&SCHEDULING_SYNC);
			pthread_mutex_lock(&(SHARED_LIST_NEW.mutex));

				if((SHARED_LIST_NEW.list)->head == NULL) {
					pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex));
					continue;
				}

				pcb = (t_PCB *) (SHARED_LIST_NEW.list)->head->data;

			pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex));

			client_thread_connect_to_server(&connection_memory);

				if(send_process_create(pcb->PID, pcb->size, connection_memory.fd_connection)) {
					// TODO
				}

				t_Return_Value return_value;
				if(receive_return_value_with_expected_header(PROCESS_CREATE_HEADER, &return_value, connection_memory.fd_connection)) {
					// TODO
				}

				if(return_value) {
					// TODO: Algo malió sal
				}

				// TODO: Sacar de la lista de NEW

			close(connection_memory.fd_connection);

			client_thread_connect_to_server(&connection_memory);

				if(send_thread_create(pcb->PID, (t_TID) 0, ((t_TCB **) (pcb->thread_manager.cb_array))[0]->pseudocode_pathname, connection_memory.fd_connection)) {
					// TODO
				}

				if(receive_expected_header(THREAD_CREATE_HEADER, connection_memory.fd_connection)) {
					// TODO
				}

				// TODO: Poner el hilo en READY
			close(connection_memory.fd_connection);

			//switch_process_state(pcb, READY_STATE);
		signal_draining_requests(&SCHEDULING_SYNC);
	}

	return NULL;
}

void *long_term_scheduler_exit(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en EXIT) iniciado");

	t_TCB *tcb;

	while(1) {
		sem_wait(&SEM_LONG_TERM_SCHEDULER_EXIT);

		wait_draining_requests(&SCHEDULING_SYNC);
			pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex));
				tcb = (t_TCB *) list_remove((SHARED_LIST_EXIT.list), 0);
			pthread_mutex_unlock(&(SHARED_LIST_EXIT.mutex));
		signal_draining_requests(&SCHEDULING_SYNC);

		// Libera recursos asignados al proceso
		/*
		while(pcb->assigned_resources->head != NULL) {

			t_Resource *resource = (t_Resource *) list_remove(pcb->assigned_resources, 0);

			pthread_mutex_lock(&(resource->mutex_instances));

			resource->instances++;
				
			if(resource->instances <= 0) {
				pthread_mutex_unlock(&(resource->mutex_instances));

				pthread_mutex_lock(&(resource->shared_list_blocked.mutex));

					if((resource->shared_list_blocked.list)->head == NULL) {
						pthread_mutex_unlock(&(resource->shared_list_blocked.mutex));
						continue;
					}

					t_PCB *pcb = (t_PCB *) list_remove(resource->shared_list_blocked.list, 0);

				pthread_mutex_unlock(&(resource->shared_list_blocked.mutex));

				list_add(pcb->assigned_resources, resource);

				switch_process_state(pcb, READY_STATE);
			}
			else {
				pthread_mutex_unlock(&(resource->mutex_instances));
			}
		}

		if(send_process_destroy(pcb->PID, CONNECTION_MEMORY.fd_connection)) {
			// TODO
			exit(1);
		}

		if(receive_expected_header(PROCESS_DESTROY_HEADER, CONNECTION_MEMORY.fd_connection)) {
			// TODO
        	exit(1);
		}
		*/

		//log_info(MINIMAL_LOGGER, "Finaliza el proceso <%d> - Motivo: <%s>", pcb->PID, EXIT_REASONS[pcb->exit_reason]);

		tid_release(&(tcb->pcb->thread_manager), tcb->TID);
		tcb_destroy(tcb);
	}

	return NULL;
}

void *cpu_interrupter(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo de interrupciones de CPU iniciado");

	e_Kernel_Interrupt kernel_interrupt;
	t_PID pid;
	t_TID tid;

	while(1) {
		if(receive_kernel_interrupt(&kernel_interrupt, &pid, &tid, CONNECTION_CPU_INTERRUPT.fd_connection)) {
			// TODO
			exit(1);
		}

		switch(kernel_interrupt) {

			case KILL_KERNEL_INTERRUPT:
				/*
				pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS);
					KILL_EXEC_PROCESS = 1;
				pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
				*/
				break;

			case QUANTUM_KERNEL_INTERRUPT:
				// TODO
				break;

			default:
				break;
		}
	}

	return NULL;
}

void *short_term_scheduler(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de corto plazo iniciado");
 
	t_TCB *tcb;
	e_Eviction_Reason eviction_reason;
	t_Payload syscall_instruction;
	int exit_status;

	while(1) {
		sem_wait(&SEM_SHORT_TERM_SCHEDULER);

		wait_draining_requests(&SCHEDULING_SYNC);

			tcb = NULL;
			switch(SCHEDULING_ALGORITHM) {

				case FIFO_SCHEDULING_ALGORITHM:

					pthread_mutex_lock(&(ARRAY_LIST_READY[0]->mutex));
						if((ARRAY_LIST_READY[0]->list)->head != NULL)
							tcb = (t_TCB *) list_remove((ARRAY_LIST_READY[0]->list), 0);
					pthread_mutex_unlock(&(ARRAY_LIST_READY[0]->mutex));

					break;
				
				case PRIORITIES_SCHEDULING_ALGORITHM:
				case MLQ_SCHEDULING_ALGORITHM:
					
					for(register t_Priority priority = 0; priority < PRIORITY_COUNT; priority++) {
						pthread_mutex_lock(&(ARRAY_LIST_READY[priority]->mutex));
							if((ARRAY_LIST_READY[priority]->list)->head != NULL) {
								tcb = (t_TCB *) list_remove((ARRAY_LIST_READY[priority]->list), 0);
								pthread_mutex_unlock(&(ARRAY_LIST_READY[priority]->mutex));
								break;
							}
						pthread_mutex_unlock(&(ARRAY_LIST_READY[priority]->mutex));
					}
					
					break;

			}
			if(tcb == NULL) {
				signal_draining_requests(&SCHEDULING_SYNC);
				continue;
			}

			switch_process_state(tcb, EXEC_STATE);

			EXEC_TCB = 1;
			while(EXEC_TCB) {

				if(send_pid_and_tid_with_header(THREAD_DISPATCH_HEADER, tcb->pcb->PID, tcb->TID, CONNECTION_CPU_DISPATCH.fd_connection)) {
					// TODO
					exit(1);
				}

				signal_draining_requests(&SCHEDULING_SYNC);

				switch(SCHEDULING_ALGORITHM) {

					case FIFO_SCHEDULING_ALGORITHM:
					case PRIORITIES_SCHEDULING_ALGORITHM:
						break;
					
					case MLQ_SCHEDULING_ALGORITHM:
						QUANTUM_INTERRUPT = 0;
						pthread_create(&THREAD_QUANTUM_INTERRUPT, NULL, (void *(*)(void *)) start_quantum, (void *) tcb);
						break;
				
				}

				if(receive_thread_eviction(&eviction_reason, &syscall_instruction, CONNECTION_CPU_DISPATCH.fd_connection)) {
					// TODO
					exit(1);
				}

				switch(SCHEDULING_ALGORITHM) {

					case FIFO_SCHEDULING_ALGORITHM:
					case PRIORITIES_SCHEDULING_ALGORITHM:
						break;

					case MLQ_SCHEDULING_ALGORITHM:

						pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPT);
							if(!QUANTUM_INTERRUPT)
								pthread_cancel(THREAD_QUANTUM_INTERRUPT);
						pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPT);
						pthread_join(THREAD_QUANTUM_INTERRUPT, NULL);

						break;

				}

				wait_draining_requests(&SCHEDULING_SYNC);

				switch(eviction_reason) {
					case UNEXPECTED_ERROR_EVICTION_REASON:
						switch_process_state(tcb, EXIT_STATE);
						EXEC_TCB = 0;
						break;

					case SEGMENTATION_FAULT_EVICTION_REASON:
						switch_process_state(tcb, EXIT_STATE);
						EXEC_TCB = 0;
						break;

					case EXIT_EVICTION_REASON:
						switch_process_state(tcb, EXIT_STATE);
						EXEC_TCB = 0;
						break;

					case KILL_KERNEL_INTERRUPT_EVICTION_REASON:
						/*
						pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS);
							KILL_EXEC_PROCESS = 0;
						pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
						*/
						switch_process_state(tcb, EXIT_STATE);
						EXEC_TCB = 0;
						break;
						
					case SYSCALL_EVICTION_REASON:

						/*
						pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS);             
							if(KILL_EXEC_PROCESS) {
								KILL_EXEC_PROCESS = 0;
								pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
								switch_process_state(tcb, EXIT_STATE);
								EXEC_TCB = 0;
								break;
							}
						pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
						*/

						SYSCALL_TCB = tcb;
						exit_status = syscall_execute(&syscall_instruction);

						if(exit_status) {
							// La syscall se encarga de settear el e_Exit_Reason (en SYSCALL_TCB)
							switch_process_state(tcb, EXIT_STATE);
							EXEC_TCB = 0;
							break;
						}

						// La syscall se encarga de settear el e_Exit_Reason (en SYSCALL_TCB) y/o el EXEC_TCB
						break;

					case QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON:
						log_info(MINIMAL_LOGGER, "PID: <%d> - Desalojado por fin de Quantum", (int) tcb->TID);

						/*
						pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS);             
							if(KILL_EXEC_PROCESS) {
								KILL_EXEC_PROCESS = 0;
								pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
								pcb->exit_reason = INTERRUPTED_BY_USER_EXIT_REASON;
								switch_process_state(pcb, EXIT_STATE);
								EXEC_TCB = 0;
								break;
							}
						pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
						*/

						switch_process_state(tcb, READY_STATE);
						EXEC_TCB = 0;
						break;
				}
			}
		signal_draining_requests(&SCHEDULING_SYNC);
	}

	return NULL;
}

void *start_quantum(t_TCB *tcb) {
	t_Quantum quantum = tcb->quantum;

    log_trace(MODULE_LOGGER, "(%d:%d) - Hilo de interrupción por quantum de %li milisegundos iniciado", (int) tcb->pcb->PID, (int) tcb->TID, quantum);

    usleep(quantum * 1000); // en milisegundos

	// ENVIAR LA INTERRUPCIÓN SÓLO SI HAY MÁS PROCESOS EN READY
	sem_wait(&SEM_SHORT_TERM_SCHEDULER);
	sem_post(&SEM_SHORT_TERM_SCHEDULER);

	pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPT);
		QUANTUM_INTERRUPT = 1;
	pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPT);

    if(send_kernel_interrupt(QUANTUM_KERNEL_INTERRUPT, tcb->pcb->PID, tcb->TID, CONNECTION_CPU_INTERRUPT.fd_connection)) {
		// TODO
		exit(1);
	}

    log_trace(MODULE_LOGGER, "(%d:%d) - Se envia interrupcion por quantum tras %li milisegundos", (int) tcb->pcb->PID, (int) tcb->TID, quantum);

	return NULL;
}

void switch_process_state(t_TCB *tcb, e_Process_State new_state) {

	e_Process_State previous_state = tcb->current_state;

	switch(previous_state) {

		case READY_STATE:
		{
			t_Shared_List *shared_list_state = tcb->shared_list_state;
			wait_draining_requests(&READY_SYNC);
				pthread_mutex_lock(&(shared_list_state->mutex));
					list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) tcb_matches_tid, &(tcb->TID));
					tcb->shared_list_state = NULL;
				pthread_mutex_unlock(&(shared_list_state->mutex));
			signal_draining_requests(&READY_SYNC);
			break;
		}

		case EXEC_STATE:
		{
			pthread_mutex_lock(&(SHARED_LIST_EXEC.mutex));
				list_remove_by_condition_with_comparation((SHARED_LIST_EXEC.list), (bool (*)(void *, void *)) tcb_matches_tid, &(tcb->TID));
				tcb->shared_list_state = NULL;
			pthread_mutex_unlock(&(SHARED_LIST_EXEC.mutex));
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
					pthread_mutex_lock(&(ARRAY_LIST_READY[0]->mutex));
						list_add((ARRAY_LIST_READY[0]->list), tcb);
						tcb->shared_list_state = ARRAY_LIST_READY[0];
					pthread_mutex_unlock(&(ARRAY_LIST_READY[0]->mutex));
					break;
				
				case PRIORITIES_SCHEDULING_ALGORITHM:
				case MLQ_SCHEDULING_ALGORITHM:
					pthread_mutex_lock(&(ARRAY_LIST_READY[tcb->priority]->mutex));
						list_add((ARRAY_LIST_READY[tcb->priority]->list), tcb);
						tcb->shared_list_state = ARRAY_LIST_READY[tcb->priority];
					pthread_mutex_unlock(&(ARRAY_LIST_READY[tcb->priority]->mutex));
					break;
			}

			sem_post(&SEM_SHORT_TERM_SCHEDULER);

			break;
		}

		case EXEC_STATE:
		{
			pthread_mutex_lock(&(SHARED_LIST_EXEC.mutex));
				log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <EXEC>", (int) tcb->TID, STATE_NAMES[previous_state]);
				list_add((SHARED_LIST_EXEC.list), tcb);
				tcb->shared_list_state = &(SHARED_LIST_EXEC);
			pthread_mutex_unlock(&(SHARED_LIST_EXEC.mutex));
			break;
		}

		case EXIT_STATE:
		{
			pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex));
				list_add((SHARED_LIST_EXIT.list), tcb);
				tcb->shared_list_state = &(SHARED_LIST_EXIT);
			pthread_mutex_unlock(&(SHARED_LIST_EXIT.mutex));

			sem_post(&SEM_LONG_TERM_SCHEDULER_EXIT);

			break;
		}

		default:
			break;
	}
}