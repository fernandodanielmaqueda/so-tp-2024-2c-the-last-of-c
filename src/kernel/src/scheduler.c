#include "scheduler.h"

t_Shared_List SHARED_LIST_NEW;
t_Shared_List SHARED_LIST_READY;
t_Shared_List SHARED_LIST_READY_PRIORITARY;
t_Shared_List SHARED_LIST_EXEC;
t_Shared_List SHARED_LIST_EXIT;

pthread_t THREAD_LONG_TERM_SCHEDULER_NEW;
sem_t SEM_LONG_TERM_SCHEDULER_NEW;
pthread_t THREAD_LONG_TERM_SCHEDULER_EXIT;
sem_t SEM_LONG_TERM_SCHEDULER_EXIT;
pthread_t THREAD_SHORT_TERM_SCHEDULER;
sem_t SEM_SHORT_TERM_SCHEDULER;

int EXEC_PCB;

t_Scheduling_Algorithm SCHEDULING_ALGORITHMS[] = {
	[FIFO_SCHEDULING_ALGORITHM] = { .name = "FIFO" , .function_fetcher = FIFO_scheduling_algorithm},
	[RR_SCHEDULING_ALGORITHM] = { .name = "RR" , .function_fetcher = RR_scheduling_algorithm },
	[VRR_SCHEDULING_ALGORITHM] = { .name = "VRR" , .function_fetcher = VRR_scheduling_algorithm },
};

e_Scheduling_Algorithm SCHEDULING_ALGORITHM;

t_Quantum QUANTUM;
pthread_t THREAD_QUANTUM_INTERRUPT;
pthread_mutex_t MUTEX_QUANTUM_INTERRUPT;
int QUANTUM_INTERRUPT;

t_temporal *TEMPORAL_DISPATCHED;

t_Drain_Ongoing_Resource_Sync SCHEDULING_SYNC;

int find_scheduling_algorithm(char *name, e_Scheduling_Algorithm *destination) {

    if(name == NULL || destination == NULL)
        return 1;
    
    size_t scheduling_algorithms_number = sizeof(SCHEDULING_ALGORITHMS) / sizeof(SCHEDULING_ALGORITHMS[0]);
    for (register e_Scheduling_Algorithm scheduling_algorithm = 0; scheduling_algorithm < scheduling_algorithms_number; scheduling_algorithm++)
        if (strcmp(SCHEDULING_ALGORITHMS[scheduling_algorithm].name, name) == 0) {
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
	free(PCB_ARRAY);
	list_destroy_and_destroy_elements(LIST_RELEASED_PIDS, free);
}

void initialize_long_term_scheduler(void) {
	pthread_create(&THREAD_LONG_TERM_SCHEDULER_NEW, NULL, (void *(*)(void *)) long_term_scheduler_new, NULL);
	pthread_detach(THREAD_LONG_TERM_SCHEDULER_NEW);

	pthread_create(&THREAD_LONG_TERM_SCHEDULER_EXIT, NULL, (void *(*)(void *)) long_term_scheduler_exit, NULL);
	pthread_detach(THREAD_LONG_TERM_SCHEDULER_EXIT);
}

void initialize_short_term_scheduler(void) { //ESTADO RUNNIG - MULTIPROCESAMIENTO
	pthread_create(&THREAD_SHORT_TERM_SCHEDULER, NULL, (void *(*)(void *)) short_term_scheduler, NULL);
	pthread_detach(THREAD_SHORT_TERM_SCHEDULER);
}

void *long_term_scheduler_new(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en NEW) iniciado");

	t_PCB *pcb;

	while(1) {
		sem_wait(&SEM_LONG_TERM_SCHEDULER_NEW);
		//wait_multiprogramming_level();

    	wait_draining_requests(&SCHEDULING_SYNC);
			pthread_mutex_lock(&(SHARED_LIST_NEW.mutex));

				if((SHARED_LIST_NEW.list)->head == NULL) {
					pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex));
					signal_draining_requests(&SCHEDULING_SYNC);
					continue;
				}

				pcb = (t_PCB *) (SHARED_LIST_NEW.list)->head->data;
			
			pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex));

			switch_process_state(pcb, READY_STATE);
		signal_draining_requests(&SCHEDULING_SYNC);
	}

	return NULL;
}

void *long_term_scheduler_exit(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de largo plazo (en EXIT) iniciado");

	t_PCB *pcb;

	while(1) {
		sem_wait(&SEM_LONG_TERM_SCHEDULER_EXIT);

		wait_draining_requests(&SCHEDULING_SYNC);
			pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex));
				pcb = (t_PCB *) list_remove((SHARED_LIST_EXIT.list), 0);
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

		//pid_release(pcb->PID);
		pcb_destroy(pcb);
		//signal_multiprogramming_level();
	}

	return NULL;
}

void *short_term_scheduler(void *NULL_parameter) {

	log_trace(MODULE_LOGGER, "Hilo planificador de corto plazo iniciado");
 
	t_PCB *pcb;
	e_Eviction_Reason eviction_reason;
	t_Payload syscall_instruction;
	int exit_status;

	while(1) {
		sem_wait(&SEM_SHORT_TERM_SCHEDULER);

		wait_draining_requests(&SCHEDULING_SYNC);
			pcb = SCHEDULING_ALGORITHMS[SCHEDULING_ALGORITHM].function_fetcher();
			if(pcb == NULL) {
				signal_draining_requests(&SCHEDULING_SYNC);
				continue;
			}
			switch_process_state(pcb, EXEC_STATE);

			EXEC_PCB = 1;
			/*
			while(EXEC_PCB) {

				if(send_process_dispatch(pcb->exec_context, CONNECTION_CPU_DISPATCH.fd_connection)) {
					// TODO
					exit(1);
				}

				signal_draining_requests(&SCHEDULING_SYNC);

				switch(SCHEDULING_ALGORITHM) {
					case RR_SCHEDULING_ALGORITHM:
						QUANTUM_INTERRUPT = 0;
						pthread_create(&THREAD_QUANTUM_INTERRUPT, NULL, (void *(*)(void *)) start_quantum, (void *) pcb);
						break;
					case VRR_SCHEDULING_ALGORITHM:
						TEMPORAL_DISPATCHED = temporal_create();
						QUANTUM_INTERRUPT = 0;
						pthread_create(&THREAD_QUANTUM_INTERRUPT, NULL, (void *(*)(void *)) start_quantum, (void *) pcb);
						break;
					case FIFO_SCHEDULING_ALGORITHM:
						break;
				}

				if(receive_process_eviction(&(pcb->exec_context), &eviction_reason, &syscall_instruction, CONNECTION_CPU_DISPATCH.fd_connection)) {
					// TODO
        			exit(1);
				}

				switch(SCHEDULING_ALGORITHM) {
					case RR_SCHEDULING_ALGORITHM:

						pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPT);
							if(!QUANTUM_INTERRUPT)
								pthread_cancel(THREAD_QUANTUM_INTERRUPT);
						pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPT);
						pthread_join(THREAD_QUANTUM_INTERRUPT, NULL);

						break;
					case VRR_SCHEDULING_ALGORITHM:

						pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPT);
							if(!QUANTUM_INTERRUPT)
								pthread_cancel(THREAD_QUANTUM_INTERRUPT);
						pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPT);
						pthread_join(THREAD_QUANTUM_INTERRUPT, NULL);

						temporal_stop(TEMPORAL_DISPATCHED);
							cpu_burst = temporal_gettime(TEMPORAL_DISPATCHED);
						temporal_destroy(TEMPORAL_DISPATCHED);
						log_trace(MODULE_LOGGER, "PID: <%d> - Rafaga de CPU: %" PRId64, (int) pcb->PID, cpu_burst);

						pcb->exec_context.quantum -= cpu_burst;
						if(pcb->exec_context.quantum <= 0)
							pcb->exec_context.quantum = QUANTUM;

						break;
					case FIFO_SCHEDULING_ALGORITHM:
						break;
				}

				wait_draining_requests(&SCHEDULING_SYNC);

				switch(eviction_reason) {
					case UNEXPECTED_ERROR_EVICTION_REASON:
						pcb->exit_reason = UNEXPECTED_ERROR_EXIT_REASON;
						switch_process_state(pcb, EXIT_STATE);
						EXEC_PCB = 0;
						break;

					case OUT_OF_MEMORY_EVICTION_REASON:
						pcb->exit_reason = OUT_OF_MEMORY_EXIT_REASON;
						switch_process_state(pcb, EXIT_STATE);
						EXEC_PCB = 0;
						break;

					case EXIT_EVICTION_REASON:
						pcb->exit_reason = SUCCESS_EXIT_REASON;
						switch_process_state(pcb, EXIT_STATE);
						EXEC_PCB = 0;
						break;

					case KILL_KERNEL_INTERRUPT_EVICTION_REASON:
						pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS);
							KILL_EXEC_PROCESS = 0;
						pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
						pcb->exit_reason = INTERRUPTED_BY_USER_EXIT_REASON;
						switch_process_state(pcb, EXIT_STATE);
						EXEC_PCB = 0;
						break;
						
					case SYSCALL_EVICTION_REASON:

						pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS);             
							if(KILL_EXEC_PROCESS) {
								KILL_EXEC_PROCESS = 0;
								pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
								pcb->exit_reason = INTERRUPTED_BY_USER_EXIT_REASON;
								switch_process_state(pcb, EXIT_STATE);
								EXEC_PCB = 0;
								break;
							}
						pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);

						SYSCALL_PCB = pcb;
						exit_status = syscall_execute(&syscall_instruction);

						if(exit_status) {
							// La syscall se encarga de settear el e_Exit_Reason (en SYSCALL_PCB)
							switch_process_state(pcb, EXIT_STATE);
							EXEC_PCB = 0;
							break;
						}

						// La syscall se encarga de settear el e_Exit_Reason (en SYSCALL_PCB) y/o el EXEC_PCB
						break;

					case QUANTUM_KERNEL_INTERRUPT_EVICTION_REASON:
						log_info(MINIMAL_LOGGER, "PID: <%d> - Desalojado por fin de Quantum", (int) pcb->PID);

						pthread_mutex_lock(&MUTEX_KILL_EXEC_PROCESS);             
							if(KILL_EXEC_PROCESS) {
								KILL_EXEC_PROCESS = 0;
								pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);
								pcb->exit_reason = INTERRUPTED_BY_USER_EXIT_REASON;
								switch_process_state(pcb, EXIT_STATE);
								EXEC_PCB = 0;
								break;
							}
						pthread_mutex_unlock(&MUTEX_KILL_EXEC_PROCESS);

						switch_process_state(pcb, READY_STATE);
						EXEC_PCB = 0;
						break;
				}
			}
			*/
		signal_draining_requests(&SCHEDULING_SYNC);
	}

	return NULL;
}

void *start_quantum(t_PCB *pcb) {
	/*
	t_Quantum quantum = pcb->exec_context.quantum;

    log_trace(MODULE_LOGGER, "PID: <%d> - Hilo de interrupción por quantum de %li milisegundos iniciado", (int) pcb->PID, quantum);

    usleep(quantum * 1000); // en milisegundos

	// ENVIAR LA INTERRUPCIÓN SÓLO SI HAY MÁS PROCESOS EN READY
	sem_wait(&SEM_SHORT_TERM_SCHEDULER);
	sem_post(&SEM_SHORT_TERM_SCHEDULER);

	pthread_mutex_lock(&MUTEX_QUANTUM_INTERRUPT);
		QUANTUM_INTERRUPT = 1;
	pthread_mutex_unlock(&MUTEX_QUANTUM_INTERRUPT);

    if(send_kernel_interrupt(QUANTUM_KERNEL_INTERRUPT, pcb->PID, CONNECTION_CPU_INTERRUPT.fd_connection)) {
		// TODO
		exit(1);
	}

    log_trace(MODULE_LOGGER, "PID: <%d> - Se envia interrupcion por quantum tras %li milisegundos", (int) pcb->PID, quantum);
	*/

	return NULL;
}

t_PCB *FIFO_scheduling_algorithm(void) {
	t_PCB *pcb = NULL;

	pthread_mutex_lock(&(SHARED_LIST_READY.mutex));
		if((SHARED_LIST_READY.list)->head != NULL)
			pcb = (t_PCB *) list_remove((SHARED_LIST_READY.list), 0);
	pthread_mutex_unlock(&(SHARED_LIST_READY.mutex));

	return pcb;
}

t_PCB *RR_scheduling_algorithm(void) {
	t_PCB *pcb = NULL;

	pthread_mutex_lock(&(SHARED_LIST_READY.mutex));
		if((SHARED_LIST_READY.list)->head != NULL)
			pcb = (t_PCB *) list_remove((SHARED_LIST_READY.list), 0);
	pthread_mutex_unlock(&(SHARED_LIST_READY.mutex));

	return pcb;
}

t_PCB *VRR_scheduling_algorithm(void) {
	t_PCB *pcb = NULL;

	pthread_mutex_lock(&(SHARED_LIST_READY_PRIORITARY.mutex));
		if((SHARED_LIST_READY_PRIORITARY.list)->head != NULL)
			pcb = (t_PCB *) list_remove((SHARED_LIST_READY_PRIORITARY.list), 0);
	pthread_mutex_unlock(&(SHARED_LIST_READY_PRIORITARY.mutex));

	if(pcb != NULL)
		return pcb;
	
	pthread_mutex_lock(&(SHARED_LIST_READY.mutex));
		if((SHARED_LIST_READY.list)->head != NULL)
			pcb = (t_PCB *) list_remove((SHARED_LIST_READY.list), 0);
	pthread_mutex_unlock(&(SHARED_LIST_READY.mutex));

	return pcb;
}

void switch_process_state(t_PCB *pcb, e_Process_State new_state) {

	/*
	e_Process_State previous_state = pcb->current_state;

	switch(previous_state) {

		case NEW_STATE:
			pthread_mutex_lock(&(SHARED_LIST_NEW.mutex));
				list_remove_by_condition_with_comparation((SHARED_LIST_NEW.list), (bool (*)(void *, void *)) pcb_matches_pid, &(pcb->PID));
				pcb->shared_list_state = NULL;
			pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex));
			break;

		case READY_STATE:
		{
			t_Shared_List *shared_list_state = pcb->shared_list_state;
			pthread_mutex_lock(&(shared_list_state->mutex));
				list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) pcb_matches_pid, &(pcb->PID));
				pcb->shared_list_state = NULL;
			pthread_mutex_unlock(&(shared_list_state->mutex));
			break;
		}

		case EXEC_STATE:
		{
			pthread_mutex_lock(&(SHARED_LIST_EXEC.mutex));
				list_remove_by_condition_with_comparation((SHARED_LIST_EXEC.list), (bool (*)(void *, void *)) pcb_matches_pid, &(pcb->PID));
				pcb->shared_list_state = NULL;
			pthread_mutex_unlock(&(SHARED_LIST_EXEC.mutex));
			break;
		}

		default:
			break;
	}

	pcb->current_state = new_state;

	switch(new_state) {

		case READY_STATE:
		{

			switch(SCHEDULING_ALGORITHM) {

				case VRR_SCHEDULING_ALGORITHM:
					if(pcb->exec_context.quantum < QUANTUM) {
						pthread_mutex_lock(&(SHARED_LIST_READY_PRIORITARY.mutex));
							log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <READY>", (int) pcb->PID, STATE_NAMES[previous_state]);
							list_add((SHARED_LIST_READY_PRIORITARY.list), pcb);
							log_state_list(MODULE_LOGGER, "Ready Prioridad", (SHARED_LIST_READY_PRIORITARY.list));
							pcb->shared_list_state = &(SHARED_LIST_READY_PRIORITARY);
						pthread_mutex_unlock(&(SHARED_LIST_READY_PRIORITARY.mutex));					
					} else {
						pthread_mutex_lock(&(SHARED_LIST_READY.mutex));
							log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <READY>", (int) pcb->PID, STATE_NAMES[previous_state]);
							list_add((SHARED_LIST_READY.list), pcb);
							log_state_list(MODULE_LOGGER, "Cola Ready", (SHARED_LIST_READY.list));
							pcb->shared_list_state = &(SHARED_LIST_READY);
						pthread_mutex_unlock(&(SHARED_LIST_READY.mutex));
					}
					break;
				
				case RR_SCHEDULING_ALGORITHM:
				case FIFO_SCHEDULING_ALGORITHM:
					pthread_mutex_lock(&(SHARED_LIST_READY.mutex));
						log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <READY>", (int) pcb->PID, STATE_NAMES[previous_state]);
						list_add((SHARED_LIST_READY.list), pcb);
						log_state_list(MODULE_LOGGER, "Cola Ready", (SHARED_LIST_READY.list));
						pcb->shared_list_state = &(SHARED_LIST_READY);
					pthread_mutex_unlock(&(SHARED_LIST_READY.mutex));
					break;

			}

			sem_post(&SEM_SHORT_TERM_SCHEDULER);

			break;
		}

		case EXEC_STATE:
		{
			pthread_mutex_lock(&(SHARED_LIST_EXEC.mutex));
				log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <EXEC>", (int) pcb->PID, STATE_NAMES[previous_state]);
				list_add((SHARED_LIST_EXEC.list), pcb);
				pcb->shared_list_state = &(SHARED_LIST_EXEC);
			pthread_mutex_unlock(&(SHARED_LIST_EXEC.mutex));
			break;
		}

		case BLOCKED_STATE:
		{
			log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <BLOCKED>", (int) pcb->PID, STATE_NAMES[previous_state]);	
			break;
		}

		case EXIT_STATE:
		{
			pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex));
				log_info(MINIMAL_LOGGER, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <EXIT>", (int) pcb->PID, STATE_NAMES[previous_state]);
				list_add((SHARED_LIST_EXIT.list), pcb);
				pcb->shared_list_state = &(SHARED_LIST_EXIT);
			pthread_mutex_unlock(&(SHARED_LIST_EXIT.mutex));

			sem_post(&SEM_LONG_TERM_SCHEDULER_EXIT);

			break;
		}

		default:
			break;
	}
	*/
}