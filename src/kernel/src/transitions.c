/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "transitions.h"

void kill_thread(t_TCB *tcb) {

    switch(tcb->current_state) {

        // No se aplica
        case NEW_STATE:
        {
            break;
        }

        case READY_STATE:
		case BLOCKED_JOIN_STATE:
		case BLOCKED_MUTEX_STATE:
		case BLOCKED_DUMP_STATE:
		case BLOCKED_IO_READY_STATE:
		case BLOCKED_IO_EXEC_STATE:
        {
            tcb->exit_reason = KILL_EXIT_REASON;
            if(locate_and_remove_state(tcb)) {
				error_pthread();
			}
            if(insert_state_exit(tcb)) {
				error_pthread();
			}

            break;
        }

        case EXEC_STATE:
        {
            KILL_EXEC_TCB = 1;
            if(send_kernel_interrupt(KILL_KERNEL_INTERRUPT, tcb->pcb->PID, tcb->TID, CONNECTION_CPU_INTERRUPT.fd_connection)) {
                log_error(MODULE_LOGGER, "[%d] Error al enviar interrupcion por cancelacion a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_INTERRUPT.fd_connection, PORT_NAMES[CONNECTION_CPU_INTERRUPT.server_type], tcb->pcb->PID, tcb->TID);
                error_pthread();
            }
            log_trace(MODULE_LOGGER, "[%d] Se envia interrupcion por cancelacion a [Servidor] %s [PID: %u - TID: %u]", CONNECTION_CPU_INTERRUPT.fd_connection, PORT_NAMES[CONNECTION_CPU_INTERRUPT.server_type], tcb->pcb->PID, tcb->TID);
            break;
        }

        // No se hace nada
        case EXIT_STATE:
        {
            break;
        }
    }
}

// Cuando sé que el TCB existe y no fue quitado
int locate_and_remove_state(t_TCB *tcb) {
	int retval = 0, status;

	switch(tcb->current_state) {

		// No se aplica
		case NEW_STATE:
		{
			break;
		}

		case READY_STATE:
		{
			t_Shared_List *shared_list_state = (t_Shared_List *) tcb->location;

			/*
			if((status = pthread_rwlock_rdlock(&ARRAY_READY_RWLOCK))) {
				log_error_pthread_rwlock_rdlock(status);
				retval = -1;
				goto ret;
			}
			pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &ARRAY_READY_RWLOCK);

				if((status = pthread_mutex_lock(&(shared_list_state->mutex)))) {
					log_error_pthread_mutex_lock(status);
					retval = -1;
					goto cleanup_ready_rwlock;
				}
			*/
					list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) pointers_match, tcb);
					tcb->location = NULL;
			/*
				if((status = pthread_mutex_unlock(&(shared_list_state->mutex)))) {
					log_error_pthread_mutex_unlock(status);
					retval = -1;
					goto cleanup_ready_rwlock;
				}

			cleanup_ready_rwlock:
			pthread_cleanup_pop(0);
			if((status = pthread_rwlock_unlock(&ARRAY_READY_RWLOCK))) {
				log_error_pthread_rwlock_unlock(status);
				retval = -1;
				goto ret;
			}
			*/

			break;
		}

		case EXEC_STATE:
		{
			/*
			if((status = pthread_mutex_lock(&MUTEX_EXEC))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto ret;
			}
			*/
				TCB_EXEC = NULL;
				tcb->location = NULL;
			/*
			if((status = pthread_mutex_unlock(&MUTEX_EXEC))) {
				log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto ret;
			}
			*/

			break;
		}

		case BLOCKED_JOIN_STATE:
		case BLOCKED_MUTEX_STATE:
		case BLOCKED_IO_READY_STATE:
		{
			t_Shared_List *shared_list_state = (t_Shared_List *) tcb->location;

			/*
			if((status = pthread_mutex_lock(&(shared_list_state->mutex)))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto ret;
			}
			*/
				list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) pointers_match, tcb);
				tcb->location = NULL;
			/*
			if((status = pthread_mutex_unlock(&(shared_list_state->mutex)))) {
				log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto ret;
			}
			*/

			break;
		}

		case BLOCKED_DUMP_STATE:
		{
			/*
			if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto ret;
			}
			*/

				t_Dump_Memory_Petition *dump_memory_petition = list_find_by_condition_with_comparation((SHARED_LIST_BLOCKED_MEMORY_DUMP.list), (bool (*)(void *, void *)) dump_memory_petition_matches_tcb, tcb);
				dump_memory_petition->tcb = NULL;

				tcb->location = NULL;

				// Opcional: Para adicionalmente cancelar la operación de volcado de memoria
				if((dump_memory_petition->bool_thread.running)) {
					if((status = pthread_cancel(dump_memory_petition->bool_thread.thread))) {
						log_error_pthread_cancel(status);
						retval = -1;
						//goto cleanup_blocked_memory_dump_mutex;
					}
				}

			/*
			cleanup_blocked_memory_dump_mutex:
			if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
				log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto ret;
			}
			*/

			break;
		}

		case BLOCKED_IO_EXEC_STATE:
		{

			/*
			if((status = pthread_mutex_lock(&MUTEX_BLOCKED_IO_EXEC))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto ret;
			}
			*/
				TCB_BLOCKED_IO_EXEC = NULL;
				tcb->location = NULL;
			/*
			if((status = pthread_mutex_unlock(&MUTEX_BLOCKED_IO_EXEC))) {
				log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto ret;
			}
			*/

            // Opcional: Para adicionalmente cancelar la operación de entrada/salida
            if((status = pthread_mutex_lock(&MUTEX_CANCEL_IO_OPERATION))) {
                log_error_pthread_mutex_lock(status);
                retval = -1;
				goto ret;
            }
                CANCEL_IO_OPERATION = true;
                pthread_cond_signal(&COND_CANCEL_IO_OPERATION);
			if((status = pthread_mutex_unlock(&MUTEX_CANCEL_IO_OPERATION))) {
                log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto ret;
            }

			break;
		}

		// No se aplica
		case EXIT_STATE:
		{
			break;
		}
	}

	ret:
	return retval;
}

int get_state_new(t_PCB **pcb) {
	int status;

	if(((status = pthread_mutex_lock(&(SHARED_LIST_NEW.mutex))))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		if((SHARED_LIST_NEW.list)->head == NULL) {
			*pcb = NULL;
		}
		else {
			*pcb = (t_PCB *) list_remove(SHARED_LIST_NEW.list, 0);
		}
	if(((status = pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex))))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	return 0;
}

int get_state_ready(t_TCB **tcb) {
	int status;

	switch(SCHEDULING_ALGORITHM) {

		case FIFO_SCHEDULING_ALGORITHM:

			if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[0].mutex)))) {
				log_error_pthread_mutex_lock(status);
				return -1;
			}
				if((ARRAY_LIST_READY[0].list)->head == NULL) {
					*tcb = NULL;
				}
				else {
					*tcb = (t_TCB *) list_remove((ARRAY_LIST_READY[0].list), 0);
					(*tcb)->location = NULL;
				}
			if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[0].mutex)))) {
				log_error_pthread_mutex_unlock(status);
				return -1;
			}

			break;

		case PRIORITIES_SCHEDULING_ALGORITHM:
		case MLQ_SCHEDULING_ALGORITHM:
			
			for(register t_Priority priority = 0; (((*tcb) == NULL) && (priority < PRIORITY_COUNT)); priority++) {
				if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[priority].mutex)))) {
					log_error_pthread_mutex_lock(status);
					return -1;
				}
					if((ARRAY_LIST_READY[priority].list)->head == NULL) {
						*tcb = NULL;
					}
					else {
						*tcb = (t_TCB *) list_remove((ARRAY_LIST_READY[priority].list), 0);
						(*tcb)->location = NULL;
					}
				if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[priority].mutex)))) {
					log_error_pthread_mutex_unlock(status);
					return -1;
				}
			}

			break;

	}

	return 0;
}

int get_state_exec(t_TCB **tcb) {
	int status;

	if((status = pthread_mutex_lock(&MUTEX_EXEC))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		if(TCB_EXEC == NULL) {
			*tcb = NULL;
		}
		else {
			*tcb = TCB_EXEC;
			TCB_EXEC = NULL;
			(*tcb)->location = NULL;
		}
	if((status = pthread_mutex_unlock(&MUTEX_EXEC))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	return 0;
}

int get_state_blocked_mutex(t_TCB **tcb, t_Resource *resource) {

	if(resource->list_blocked->head == NULL) {
		*tcb = NULL;
	}
	else {
		*tcb = (t_TCB *) list_remove(resource->list_blocked, 0);
		(*tcb)->location = NULL;
	}

	return 0;
}

int get_state_blocked_io_ready(t_TCB **tcb) {
	int status;

	if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_IO_READY.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		if((SHARED_LIST_BLOCKED_IO_READY.list)->head == NULL) {
			*tcb = NULL;
		}
		else {
			*tcb = (t_TCB *) list_remove(SHARED_LIST_BLOCKED_IO_READY.list, 0);
			(*tcb)->location = NULL;
		}
	if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_IO_READY.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	return 0;
}

int get_state_blocked_io_exec(t_TCB **tcb) {
	int status;

	if((status = pthread_mutex_lock(&MUTEX_BLOCKED_IO_EXEC))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		if(TCB_BLOCKED_IO_EXEC == NULL) {
			*tcb = NULL;
		}
		else {
			*tcb = TCB_BLOCKED_IO_EXEC;
			TCB_BLOCKED_IO_EXEC = NULL;
			(*tcb)->location = NULL;
		}
	if((status = pthread_mutex_unlock(&MUTEX_BLOCKED_IO_EXEC))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	return 0;
}

int get_state_exit(t_TCB **tcb) {
	int status;

	if((status = pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		if((SHARED_LIST_EXIT.list)->head == NULL) {
			*tcb = NULL;
		}
		else {
			*tcb = (t_TCB *) list_remove((SHARED_LIST_EXIT.list), 0);
			(*tcb)->location = NULL;
		}
	if((status = pthread_mutex_unlock(&(SHARED_LIST_EXIT.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	return 0;
}

int insert_state_new(t_PCB *pcb) {
	int status;

	if((status = pthread_mutex_lock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		list_add(SHARED_LIST_NEW.list, pcb);
	if((status = pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	log_info(MINIMAL_LOGGER, "## (%u:0) Se crea el proceso - Estado: NEW", pcb->PID);

	if(sem_post(&SEM_LONG_TERM_SCHEDULER_NEW)) {
		log_error_sem_post();
		return -1;
	}

	return 0;
}

int insert_state_ready(t_TCB *tcb) {
	int retval = 0, status;

	e_Process_State previous_state = tcb->current_state;
	tcb->current_state = READY_STATE;

	if((status = pthread_rwlock_rdlock(&ARRAY_READY_RWLOCK))) {
		log_error_pthread_rwlock_rdlock(status);
		return -1;
	}
	pthread_cleanup_push((void (*)(void *)) pthread_rwlock_unlock, &ARRAY_READY_RWLOCK);

		switch(SCHEDULING_ALGORITHM) {

			case FIFO_SCHEDULING_ALGORITHM:
				if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[0].mutex)))) {
					log_error_pthread_mutex_lock(status);
					retval = -1;
					goto cleanup_ready_rwlock;
				}
					list_add((ARRAY_LIST_READY[0].list), tcb);
					tcb->location = &(ARRAY_LIST_READY[0]);
				if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[0].mutex)))) {
					log_error_pthread_mutex_unlock(status);
					retval = -1;
					goto cleanup_ready_rwlock;
				}
				break;

			case PRIORITIES_SCHEDULING_ALGORITHM:
			case MLQ_SCHEDULING_ALGORITHM:
				if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[tcb->priority].mutex)))) {
					log_error_pthread_mutex_lock(status);
					retval = -1;
					goto cleanup_ready_rwlock;
				}
					list_add((ARRAY_LIST_READY[tcb->priority].list), tcb);
					tcb->location = &(ARRAY_LIST_READY[tcb->priority]);
				if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[tcb->priority].mutex)))) {
					log_error_pthread_mutex_unlock(status);
					retval = -1;
					goto cleanup_ready_rwlock;
				}
				break;
		}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: READY", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);

	if(sem_post(&SEM_SHORT_TERM_SCHEDULER)) {
		log_error_sem_post();
		retval = -1;
		goto cleanup_ready_rwlock;
	}

	cleanup_ready_rwlock:
	pthread_cleanup_pop(0);
	if((status = pthread_rwlock_unlock(&ARRAY_READY_RWLOCK))) {
		log_error_pthread_rwlock_unlock(status);
		retval = -1;
		goto ret;
	}

	ret:
	return retval;
}

int insert_state_exec(t_TCB *tcb) {
	int status;

	e_Process_State previous_state = tcb->current_state;
	tcb->current_state = EXEC_STATE;

	if((status = pthread_mutex_lock(&MUTEX_EXEC))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		TCB_EXEC = tcb;
		tcb->location = &TCB_EXEC;
	if((status = pthread_mutex_unlock(&MUTEX_EXEC))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: EXEC", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);

	return 0;
}

int insert_state_blocked_join(t_TCB *tcb, t_TCB *target) {
	int status;

	e_Process_State previous_state = tcb->current_state;
	tcb->current_state = BLOCKED_JOIN_STATE;

	if((status = pthread_mutex_lock(&(target->shared_list_blocked_thread_join.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		list_add(target->shared_list_blocked_thread_join.list, tcb);
		tcb->location = &(target->shared_list_blocked_thread_join);
	if((status = pthread_mutex_unlock(&(target->shared_list_blocked_thread_join.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: BLOCKED_JOIN", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);
	log_info(MINIMAL_LOGGER, "## (%u:%u) - Bloqueado por: THREAD_JOIN", tcb->pcb->PID, tcb->TID);

	return 0;
}

int insert_state_blocked_mutex(t_TCB *tcb, t_Resource *resource) {

	e_Process_State previous_state = tcb->current_state;
	tcb->current_state = BLOCKED_MUTEX_STATE;

	list_add(resource->list_blocked, tcb);
	tcb->location = resource;

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: BLOCKED_MUTEX", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);
	log_info(MINIMAL_LOGGER, "## (%u:%u) - Bloqueado por: MUTEX", tcb->pcb->PID, tcb->TID);

	return 0;
}

int insert_state_blocked_dump_memory(t_Dump_Memory_Petition *dump_memory_petition) {
	int retval = 0, status;

	e_Process_State previous_state = dump_memory_petition->tcb->current_state;
	dump_memory_petition->tcb->current_state = BLOCKED_DUMP_STATE;

	bool join = false;
	t_Bool_Join_Thread join_thread = { .thread = &(dump_memory_petition->bool_thread.thread), .join = &join };
	pthread_cleanup_push((void (*)(void *)) wrapper_pthread_join, &join_thread);

		if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
			log_error_pthread_mutex_lock(status);
			retval = -1;
			goto cleanup_join;
		}
		pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex));

			if((status = pthread_create(&(dump_memory_petition->bool_thread.thread), NULL, (void *(*)(void *)) dump_memory_petitioner, NULL))) {
				log_error_pthread_create(status);
				retval = -1;
				goto cleanup_blocked_memory_dump_mutex;
			}
			pthread_cleanup_push((void (*)(void *)) wrapper_pthread_cancel, &(dump_memory_petition->bool_thread.thread));

				join = true;
					if((status = pthread_detach(dump_memory_petition->bool_thread.thread))) {
						log_error_pthread_detach(status);
						retval = -1;
						goto cleanup_cancel_thread;
					}
				join = false;

			cleanup_cancel_thread:
			pthread_cleanup_pop(0);
			if(retval) {
				if((status = pthread_cancel(dump_memory_petition->bool_thread.thread))) {
					join = false;
					log_error_pthread_cancel(status);
				}
				goto cleanup_blocked_memory_dump_mutex;
			}

			dump_memory_petition->bool_thread.running = true;

			list_add((SHARED_LIST_BLOCKED_MEMORY_DUMP.list), dump_memory_petition);
			dump_memory_petition->tcb->location = &SHARED_LIST_BLOCKED_MEMORY_DUMP;

		cleanup_blocked_memory_dump_mutex:
		pthread_cleanup_pop(0);
		if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
			join = false;
			log_error_pthread_mutex_unlock(status);
			retval = -1;
		}

	cleanup_join:
	pthread_cleanup_pop(0);
	if(retval) {
		wrapper_pthread_join(&join_thread);
		return -1;
	}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: BLOCKED_DUMP", dump_memory_petition->tcb->pcb->PID, dump_memory_petition->tcb->TID, STATE_NAMES[previous_state]);
	log_info(MINIMAL_LOGGER, "## (%u:%u) - Bloqueado por: DUMP_MEMORY", dump_memory_petition->tcb->pcb->PID, dump_memory_petition->tcb->TID);

	return 0;
}

int insert_state_blocked_io_ready(t_TCB *tcb) {
	int status;

	e_Process_State previous_state = tcb->current_state;
	tcb->current_state = BLOCKED_IO_READY_STATE;

	if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_IO_READY.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		list_add((SHARED_LIST_BLOCKED_IO_READY.list), tcb);
		tcb->location = &SHARED_LIST_BLOCKED_IO_READY;
	if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_IO_READY.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: BLOCKED_IO_READY", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);
	log_info(MINIMAL_LOGGER, "## (%u:%u) - Bloqueado por: IO", tcb->pcb->PID, tcb->TID);

	if(sem_post(&SEM_IO_DEVICE)) {
		log_error_sem_post();
		return -1;
	}

	return 0;
}

int insert_state_blocked_io_exec(t_TCB *tcb) {
	int status;

	e_Process_State previous_state = tcb->current_state;
	tcb->current_state = BLOCKED_IO_EXEC_STATE;

	if(((status = pthread_mutex_lock(&MUTEX_BLOCKED_IO_EXEC)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		TCB_BLOCKED_IO_EXEC = tcb;
		TCB_BLOCKED_IO_EXEC->location = &TCB_BLOCKED_IO_EXEC;
	if(((status = pthread_mutex_unlock(&MUTEX_BLOCKED_IO_EXEC)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: BLOCKED_IO_EXEC", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);

	return 0;
}

int insert_state_exit(t_TCB *tcb) {
	int status;

	e_Process_State previous_state = tcb->current_state;
	tcb->current_state = EXIT_STATE;

	if((status = pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		list_add((SHARED_LIST_EXIT.list), tcb);
		tcb->location = &SHARED_LIST_EXIT;
	if((status = pthread_mutex_unlock(&(SHARED_LIST_EXIT.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: EXIT", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);

	if(sem_post(&SEM_LONG_TERM_SCHEDULER_EXIT)) {
		log_error_sem_post();
		return -1;
	}

	return 0;
}

int reinsert_state_new(t_PCB *pcb) {
	int status;

	if((status = pthread_mutex_lock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		list_add_in_index(SHARED_LIST_NEW.list, 0, pcb);
	if((status = pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex)))) {
		log_error_pthread_mutex_unlock(status);
		return -1;
	}

	return 0;
}