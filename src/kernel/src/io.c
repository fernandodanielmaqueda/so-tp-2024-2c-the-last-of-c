/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "io.h"

t_PThread_Controller THREAD_IO_DEVICE = { .was_created = false };
sem_t SEM_IO_DEVICE;
t_Shared_List SHARED_LIST_IO_BLOCKED; // Lista de TCBs
bool CANCEL_IO_OPERATION = false;

void sigusr1_handler(int signal) {
	CANCEL_IO_OPERATION = true;
}

void *io_device(void *NULL_parameter) {
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

	if(sigaction(SIGUSR1, &(struct sigaction) {.sa_handler = sigusr1_handler}, NULL)) {
		log_error_sigaction();
		error_pthread();
	}

	log_trace(MODULE_LOGGER, "Hilo de IO iniciado");

	t_TCB *tcb;

	while(1) {
		if(sem_wait(&SEM_IO_DEVICE)) {
			log_error_sem_wait();
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) sem_post, &SEM_IO_DEVICE);

			if(((status = pthread_mutex_lock(&(SHARED_LIST_IO_BLOCKED.mutex))))) {
				log_error_pthread_mutex_lock(status);
				error_pthread();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_IO_BLOCKED.mutex));

				if((SHARED_LIST_IO_BLOCKED.list)->head == NULL) {
					status = -1; // La lista estaba vacía
				}
				else {
					tcb = (t_TCB *) (SHARED_LIST_IO_BLOCKED.list)->head->data;
				}

			pthread_cleanup_pop(1);

			if(status) {
				goto cleanup;
			}

				t_Time time;
			payload_remove(&(EXEC_TCB->syscall_instruction), &time, sizeof(time));

			if((status = pthread_sigmask(SIG_UNBLOCK, &set_SIGUSR1, NULL))) {
				log_error_pthread_sigmask(status);
				error_pthread();
			}
			CANCEL_IO_OPERATION = false;

			usleep(time * 1000); // ms -> us

			if((status = pthread_sigmask(SIG_BLOCK, &set_SIGUSR1, NULL))) {
				log_error_pthread_sigmask(status);
				error_pthread();
			}

			if(CANCEL_IO_OPERATION) {
				goto cleanup;
			}

        if(wait_draining_requests(&SCHEDULING_SYNC)) {
			error_pthread();
		}
		pthread_cleanup_push((void (*)(void *)) signal_draining_requests, &SCHEDULING_SYNC);

			if((status = pthread_mutex_lock(&(SHARED_LIST_IO_BLOCKED.mutex)))) {
				log_error_pthread_mutex_lock(status);
				error_pthread();
			}
			pthread_cleanup_push((void (*)(void *)) pthread_mutex_unlock, &(SHARED_LIST_IO_BLOCKED.mutex));

				tcb = (t_TCB *) list_remove_by_condition_with_comparation(SHARED_LIST_IO_BLOCKED.list, (bool (*)(void *, void *)) tcb_matches_tid, &(tcb->TID));
				if(tcb != NULL) {
					tcb->shared_list_state = NULL;
					
					//payload_destroy(&(tcb->syscall_instruction));

					switch_process_state(tcb, READY_STATE);
				}

			pthread_cleanup_pop(1);
        pthread_cleanup_pop(1);

		cleanup:
			pthread_cleanup_pop(status);
	}

	pthread_exit(NULL);
}