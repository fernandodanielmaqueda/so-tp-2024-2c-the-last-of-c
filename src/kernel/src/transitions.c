/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "transitions.h"

void kill_thread(t_TCB *tcb) {
    int status;

    switch(tcb->current_state) {

        // No se aplica
        case NEW_STATE:
        {
            break;
        }

        case READY_STATE:
        {
            tcb->exit_reason = KILL_EXIT_REASON;
            if(locate_and_remove_state(tcb)) {
				error_pthread();
			}
            if(insert_state_exit(tcb, tcb->current_state)) {
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

        case BLOCKED_JOIN_STATE:
        {
            // TODO

            break;
        }

        case BLOCKED_MUTEX_STATE:
        {
            // TODO
            /*
            pcb->exit_reason = KILL_EXIT_REASON;

            t_Shared_List *shared_list_state = (t_Shared_List *) pcb->location;
            pthread_mutex_lock(&(shared_list_state->mutex));
                list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) pcb_matches_pid, &(pcb->exec_context.PID));
                pcb->location = NULL;
            pthread_mutex_unlock(&(shared_list_state->mutex));

            switch_process_state(pcb, EXIT_STATE);
            */

            break;
        }

        case BLOCKED_DUMP_STATE:
        {
            // TODO

            break;
        }

        case BLOCKED_IO_READY_STATE:
        {
            // TODO

            break;
        }

        case BLOCKED_IO_EXEC_STATE:
        {
            tcb->exit_reason = KILL_EXIT_REASON;
            if(locate_and_remove_state(tcb)) {
				error_pthread();
			}
            if(insert_state_exit(tcb, BLOCKED_IO_EXEC_STATE)) {
				error_pthread();
			}

            // Opcional: Para adicionalmente cancelar la operación de entrada/salida
            if((status = pthread_mutex_lock(&MUTEX_CANCEL_IO_OPERATION))) {
                log_error_pthread_mutex_lock(status);
                error_pthread();
            }
                CANCEL_IO_OPERATION = true;
                pthread_cond_signal(&COND_CANCEL_IO_OPERATION);
			if((status = pthread_mutex_unlock(&MUTEX_CANCEL_IO_OPERATION))) {
                log_error_pthread_mutex_unlock(status);
                error_pthread();
            }

            break;
        }

        // No se hace nada
        case EXIT_STATE:
        {
            break;
        }
    }

}

int get_state_new(t_PCB **pcb) {
	int status;

	if(((status = pthread_mutex_lock(&(SHARED_LIST_NEW.mutex))))) {
		log_error_pthread_mutex_lock(status);
		goto error;
	}
		if((SHARED_LIST_NEW.list)->head == NULL) {
			*pcb = NULL;
		}
		else {
			*pcb = (t_PCB *) list_remove(SHARED_LIST_NEW.list, 0);
		}
	if(((status = pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex))))) {
		log_error_pthread_mutex_unlock(status);
		goto error_pcb_destroy;
	}

	return 0;

	error_pcb_destroy:
	if(*pcb != NULL) {
		pcb_destroy(*pcb);
	}
	error:
	return -1;
}

int get_state_ready(t_TCB **tcb) {
	int status;

	switch(SCHEDULING_ALGORITHM) {

		case FIFO_SCHEDULING_ALGORITHM:

			if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[0].mutex)))) {
				log_error_pthread_mutex_lock(status);
				goto error;
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
				goto error_tcb_destroy;
			}

			break;

		case PRIORITIES_SCHEDULING_ALGORITHM:
		case MLQ_SCHEDULING_ALGORITHM:
			
			for(register t_Priority priority = 0; (((*tcb) == NULL) && (priority < PRIORITY_COUNT)); priority++) {
				if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[priority].mutex)))) {
					log_error_pthread_mutex_lock(status);
					goto error;
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
					goto error_tcb_destroy;
				}
			}

			break;

	}

	return 0;

	error_tcb_destroy:
	if(*tcb != NULL) {
		tcb_destroy(*tcb);
	}
	error:
	return -1;
}

int get_state_blocked_io_ready(t_TCB **tcb) {
	int status;

	if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_IO_READY.mutex)))) {
		log_error_pthread_mutex_lock(status);
		goto error;
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
		goto error_tcb_destroy;
	}

	return 0;

	error_tcb_destroy:
	if(*tcb != NULL) {
		tcb_destroy(*tcb);
	}
	error:
	return -1;
}

int get_state_blocked_io_exec(t_TCB **tcb) {
	int status;

	if((status = pthread_mutex_lock(&MUTEX_BLOCKED_IO_EXEC))) {
		log_error_pthread_mutex_lock(status);
		goto error;
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
		goto error_tcb_destroy;
	}

	return 0;

	error_tcb_destroy:
	if(*tcb != NULL) {
		tcb_destroy(*tcb);
	}
	error:
	return -1;
}

int get_state_exit(t_TCB **tcb) {
	int status;

	if((status = pthread_mutex_lock(&(SHARED_LIST_EXIT.mutex)))) {
		log_error_pthread_mutex_lock(status);
		goto error;
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
		goto error_tcb_destroy;
	}

	return 0;

	error_tcb_destroy:
	if(*tcb != NULL) {
		tcb_destroy(*tcb);
	}
	error:
	return -1;
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

			if((status = pthread_rwlock_rdlock(&ARRAY_READY_RWLOCK))) {
				log_error_pthread_rwlock_rdlock(status);
				retval = -1;
				goto ret;
			}
				if((status = pthread_mutex_lock(&(shared_list_state->mutex)))) {
					PTHREAD_SETCANCELSTATE_DISABLE();
						log_error_pthread_mutex_lock(status);
					PTHREAD_SETCANCELSTATE_OLDSTATE();
					retval = -1;
					goto cleanup_ready_rwlock;
				}
					list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) tcb_matches_tid, &(tcb->TID));
					tcb->location = NULL;
				if((status = pthread_mutex_unlock(&(shared_list_state->mutex)))) {
					PTHREAD_SETCANCELSTATE_DISABLE();
						log_error_pthread_mutex_unlock(status);
					PTHREAD_SETCANCELSTATE_OLDSTATE();
					retval = -1;
					goto cleanup_ready_rwlock;
				}
			cleanup_ready_rwlock:
			if((status = pthread_rwlock_unlock(&ARRAY_READY_RWLOCK))) {
				log_error_pthread_rwlock_unlock(status);
				retval = -1;
				goto ret;
			}

			break;
		}

		case EXEC_STATE:
		{
			if((status = pthread_mutex_lock(&MUTEX_EXEC))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto ret;
			}
				TCB_EXEC = NULL;
				tcb->location = NULL;
			if((status = pthread_mutex_unlock(&MUTEX_EXEC))) {
				log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto ret;
			}

			break;
		}

		case BLOCKED_JOIN_STATE:
		{
			t_Shared_List *shared_list_state = (t_Shared_List *) tcb->location;

			if((status = pthread_mutex_lock(&(shared_list_state->mutex)))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto ret;
			}
				list_remove_by_condition_with_comparation((shared_list_state->list), (bool (*)(void *, void *)) tcb_matches_tid, &(tcb->TID));
				tcb->location = NULL;
			if((status = pthread_mutex_unlock(&(shared_list_state->mutex)))) {
				log_error_pthread_mutex_unlock(status);
				retval = -1;
				goto ret;
			}

			break;
		}

		case BLOCKED_MUTEX_STATE:
		{
			// TODO
			break;
		}

		case BLOCKED_DUMP_STATE:
		{
			// TODO
			break;
		}

		case BLOCKED_IO_READY_STATE:
		{
			// TODO
			break;
		}

		case BLOCKED_IO_EXEC_STATE:
		{
			if((status = pthread_mutex_lock(&MUTEX_BLOCKED_IO_EXEC))) {
				log_error_pthread_mutex_lock(status);
				retval = -1;
				goto ret;
			}
				TCB_BLOCKED_IO_EXEC = NULL;
				tcb->location = NULL;
			if((status = pthread_mutex_unlock(&MUTEX_BLOCKED_IO_EXEC))) {
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

int insert_state_ready(t_TCB *tcb, e_Process_State previous_state) {
	int retval = 0, status;

	tcb->current_state = READY_STATE;

	if((status = pthread_rwlock_rdlock(&ARRAY_READY_RWLOCK))) {
		log_error_pthread_rwlock_rdlock(status);
		return -1;
	}
		switch(SCHEDULING_ALGORITHM) {

			case FIFO_SCHEDULING_ALGORITHM:
				if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[0].mutex)))) {
					PTHREAD_SETCANCELSTATE_DISABLE();
						log_error_pthread_mutex_lock(status);
					PTHREAD_SETCANCELSTATE_OLDSTATE();
					retval = -1;
					goto cleanup_ready_rwlock;
				}
					list_add((ARRAY_LIST_READY[0].list), tcb);
					tcb->location = &(ARRAY_LIST_READY[0]);
				if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[0].mutex)))) {
					PTHREAD_SETCANCELSTATE_DISABLE();
						log_error_pthread_mutex_unlock(status);
					PTHREAD_SETCANCELSTATE_OLDSTATE();
					retval = -1;
					goto cleanup_ready_rwlock;
				}
				break;

			case PRIORITIES_SCHEDULING_ALGORITHM:
			case MLQ_SCHEDULING_ALGORITHM:
				if((status = pthread_mutex_lock(&(ARRAY_LIST_READY[tcb->priority].mutex)))) {
					PTHREAD_SETCANCELSTATE_DISABLE();
						log_error_pthread_mutex_lock(status);
					PTHREAD_SETCANCELSTATE_OLDSTATE();
					retval = -1;
					goto cleanup_ready_rwlock;
				}
					list_add((ARRAY_LIST_READY[tcb->priority].list), tcb);
					tcb->location = &(ARRAY_LIST_READY[tcb->priority]);
				if((status = pthread_mutex_unlock(&(ARRAY_LIST_READY[tcb->priority].mutex)))) {
					PTHREAD_SETCANCELSTATE_DISABLE();
						log_error_pthread_mutex_unlock(status);
					PTHREAD_SETCANCELSTATE_OLDSTATE();
					retval = -1;
					goto cleanup_ready_rwlock;
				}
				break;
		}

	PTHREAD_SETCANCELSTATE_DISABLE();
		log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: READY", tcb->pcb->PID, tcb->TID, STATE_NAMES[previous_state]);
	PTHREAD_SETCANCELSTATE_OLDSTATE();

	if(sem_post(&SEM_SHORT_TERM_SCHEDULER)) {
		PTHREAD_SETCANCELSTATE_DISABLE();
			log_error_sem_post();
		PTHREAD_SETCANCELSTATE_OLDSTATE();
		retval = -1;
		goto cleanup_ready_rwlock;
	}

	cleanup_ready_rwlock:
	if((status = pthread_rwlock_unlock(&ARRAY_READY_RWLOCK))) {
		log_error_pthread_rwlock_unlock(status);
		retval = -1;
		goto ret;
	}

	ret:
	return retval;
}

int insert_state_exec(t_TCB *tcb, e_Process_State previous_state) {
	int status;

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

int insert_state_blocked_join(t_TCB *tcb, t_TCB *target, e_Process_State previous_state) {
	int status;

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

int insert_state_blocked_dump_memory(t_Dump_Memory_Petition *dump_memory_petition, e_Process_State previous_state) {
	int status;

	dump_memory_petition->tcb->current_state = BLOCKED_DUMP_STATE;

	if((status = pthread_mutex_lock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
		log_error_pthread_mutex_lock(status);
		return -1;
	}
		list_add((SHARED_LIST_BLOCKED_MEMORY_DUMP.list), dump_memory_petition);
		dump_memory_petition->tcb->location = &SHARED_LIST_BLOCKED_MEMORY_DUMP;
	if((status = pthread_mutex_unlock(&(SHARED_LIST_BLOCKED_MEMORY_DUMP.mutex)))) {
		PTHREAD_SETCANCELSTATE_DISABLE();
			log_error_pthread_mutex_unlock(status);
		PTHREAD_SETCANCELSTATE_OLDSTATE();
		list_remove_by_condition_with_comparation((SHARED_LIST_BLOCKED_MEMORY_DUMP.list), (bool (*)(void *, void *)) pointers_match, dump_memory_petition);
		return -1;
	}

	log_info(MODULE_LOGGER, "(%u:%u): Estado Anterior: %s - Estado Actual: BLOCKED_DUMP", dump_memory_petition->tcb->pcb->PID, dump_memory_petition->tcb->TID, STATE_NAMES[previous_state]);
	log_info(MINIMAL_LOGGER, "## (%u:%u) - Bloqueado por: DUMP_MEMORY", dump_memory_petition->tcb->pcb->PID, dump_memory_petition->tcb->TID);

	return 0;
}

int insert_state_blocked_io_ready(t_TCB *tcb, e_Process_State previous_state) {
	int status;

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

int insert_state_blocked_io_exec(t_TCB *tcb, e_Process_State previous_state) {
	int status;

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

int insert_state_exit(t_TCB *tcb, e_Process_State previous_state) {
	int status;

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