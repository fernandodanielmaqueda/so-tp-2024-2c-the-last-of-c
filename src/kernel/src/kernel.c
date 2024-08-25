/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "kernel.h"

char *MODULE_NAME = "kernel";

t_log *MODULE_LOGGER;
char *MODULE_LOG_PATHNAME = "kernel.log";

t_config *MODULE_CONFIG;
char *MODULE_CONFIG_PATHNAME = "kernel.config";

t_PID PID_COUNTER = 0;
pthread_mutex_t MUTEX_PID_COUNTER;
t_PCB **PCB_ARRAY = NULL;
pthread_mutex_t MUTEX_PCB_ARRAY;
t_list *LIST_RELEASED_PIDS; // LIFO
pthread_mutex_t MUTEX_LIST_RELEASED_PIDS;
pthread_cond_t COND_LIST_RELEASED_PIDS;

const char *STATE_NAMES[] = {
	[NEW_STATE] = "NEW",
	[READY_STATE] = "READY",
	[EXEC_STATE] = "EXEC",
	[BLOCKED_STATE] = "BLOCKED",
	[EXIT_STATE] = "EXIT"
};

const char *EXIT_REASONS[] = {
	[UNEXPECTED_ERROR_EXIT_REASON] = "UNEXPECTED ERROR",

	[SUCCESS_EXIT_REASON] = "SUCCESS",
	[INVALID_RESOURCE_EXIT_REASON] = "INVALID_RESOURCE",
	[INVALID_INTERFACE_EXIT_REASON] = "INVALID_INTERFACE",
	[OUT_OF_MEMORY_EXIT_REASON] = "OUT_OF_MEMORY",
	[INTERRUPTED_BY_USER_EXIT_REASON] = "INTERRUPTED_BY_USER"
};

int module(int argc, char *argv[]) {

	if(argc < 3) {
		fprintf(stderr, "Uso: %s <ARCHIVO_PSEUDOCODIGO> <TAMANIO_PROCESO> [ARGUMENTOS]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	initialize_configs(MODULE_CONFIG_PATHNAME);
	initialize_loggers();
	
	// initialize_global_variables();
	init_resource_sync(&SCHEDULING_SYNC);
	initialize_mutexes();
	initialize_semaphores();
	LIST_RELEASED_PIDS = list_create();

	SHARED_LIST_NEW.list = list_create();
	SHARED_LIST_READY.list = list_create();
	SHARED_LIST_READY_PRIORITARY.list = list_create();
	SHARED_LIST_EXEC.list = list_create();
	SHARED_LIST_EXIT.list = list_create();

	initialize_sockets();
	initialize_scheduling();

	log_debug(MODULE_LOGGER, "Modulo %s inicializado correctamente\n", MODULE_NAME);

	finish_scheduling();
	//finish_threads();
	finish_sockets();
	destroy_resource_sync(&SCHEDULING_SYNC);
	//finish_configs();
	finish_loggers();
	finish_semaphores();
	finish_mutexes();

    return EXIT_SUCCESS;
}

void initialize_mutexes(void) {
	pthread_mutex_init(&MUTEX_PID_COUNTER, NULL);
	pthread_mutex_init(&MUTEX_PCB_ARRAY, NULL);
	pthread_mutex_init(&MUTEX_LIST_RELEASED_PIDS, NULL);
	pthread_cond_init(&COND_LIST_RELEASED_PIDS, NULL);

	pthread_mutex_init(&(SHARED_LIST_NEW.mutex), NULL);
	pthread_mutex_init(&(SHARED_LIST_READY.mutex), NULL);
	pthread_mutex_init(&(SHARED_LIST_READY_PRIORITARY.mutex), NULL);
	pthread_mutex_init(&(SHARED_LIST_EXEC.mutex), NULL);
	pthread_mutex_init(&(SHARED_LIST_EXIT.mutex), NULL);

	pthread_mutex_init(&MUTEX_QUANTUM_INTERRUPT, NULL);
}

void finish_mutexes(void) {
	pthread_mutex_destroy(&MUTEX_PID_COUNTER);
	pthread_mutex_destroy(&MUTEX_PCB_ARRAY);
	pthread_mutex_destroy(&MUTEX_LIST_RELEASED_PIDS);
	pthread_cond_destroy(&COND_LIST_RELEASED_PIDS);

	pthread_mutex_destroy(&(SHARED_LIST_NEW.mutex));
	pthread_mutex_destroy(&(SHARED_LIST_READY.mutex));
	pthread_mutex_destroy(&(SHARED_LIST_READY_PRIORITARY.mutex));
	pthread_mutex_destroy(&(SHARED_LIST_EXEC.mutex));
	pthread_mutex_destroy(&(SHARED_LIST_EXIT.mutex));

	pthread_mutex_destroy(&MUTEX_QUANTUM_INTERRUPT);
}

void initialize_semaphores(void) {

	sem_init(&SEM_LONG_TERM_SCHEDULER_NEW, 0, 0);
	sem_init(&SEM_LONG_TERM_SCHEDULER_EXIT, 0, 0);
	sem_init(&SEM_SHORT_TERM_SCHEDULER, 0, 0);
}

void finish_semaphores(void) {
	sem_destroy(&SEM_LONG_TERM_SCHEDULER_NEW);
	sem_destroy(&SEM_LONG_TERM_SCHEDULER_EXIT);
	sem_destroy(&SEM_SHORT_TERM_SCHEDULER);
}

void read_module_config(t_config *module_config) {
	//CONNECTION_MEMORY = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = MEMORY_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_MEMORIA"), .port = config_get_string_value(module_config, "PUERTO_MEMORIA")};
	CONNECTION_CPU_DISPATCH = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = CPU_DISPATCH_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_DISPATCH")};
	CONNECTION_CPU_INTERRUPT = (t_Connection) {.client_type = KERNEL_PORT_TYPE, .server_type = CPU_INTERRUPT_PORT_TYPE, .ip = config_get_string_value(module_config, "IP_CPU"), .port = config_get_string_value(module_config, "PUERTO_CPU_INTERRUPT")};
	
	if(find_scheduling_algorithm(config_get_string_value(module_config, "ALGORITMO_PLANIFICACION"), &SCHEDULING_ALGORITHM)) {
		log_error(MODULE_LOGGER, "ALGORITMO_PLANIFICACION invalido");
		exit(EXIT_FAILURE);
	}

	QUANTUM = config_get_int_value(module_config, "QUANTUM");
	LOG_LEVEL = log_level_from_string(config_get_string_value(module_config, "LOG_LEVEL"));
}

t_PCB *pcb_create(void) {

	t_PCB *pcb = malloc(sizeof(t_PCB));
	if(pcb == NULL) {
		log_error(MODULE_LOGGER, "No se pudo reservar memoria para el PCB");
		exit(EXIT_FAILURE);
	}

	pcb->PID = pid_assign(pcb);
	//pcb->list_tids = list_create();
	pcb->list_mutexes = list_create();

	//pcb->current_state = NEW_STATE;
	//pcb->shared_list_state = NULL;

	//pcb->exec_context.quantum = QUANTUM;

	//payload_init(&(pcb->syscall_instruction));

	return pcb;
}

void pcb_destroy(t_PCB *pcb) {
	//list_destroy(pcb->assigned_resources);

	//payload_destroy(&(pcb->syscall_instruction));

	free(pcb);
}

t_PID pid_assign(t_PCB *pcb) {

	pthread_mutex_lock(&MUTEX_LIST_RELEASED_PIDS);
		if(LIST_RELEASED_PIDS->head == NULL && PID_COUNTER <= PID_MAX) {
			// Si no hay PID liberados y no se alcanzó el máximo de PID, se asigna un nuevo PID
			pthread_mutex_unlock(&MUTEX_LIST_RELEASED_PIDS);

			pthread_mutex_lock(&MUTEX_PCB_ARRAY);
				t_PCB **new_pcb_array = realloc(PCB_ARRAY, sizeof(t_PCB *) * (PID_COUNTER + 1));
				if(new_pcb_array == NULL) {
					log_error(MODULE_LOGGER, "No se pudo reservar memoria para el array de PCBs");
					exit(EXIT_FAILURE);
				}
				PCB_ARRAY = new_pcb_array;

				PCB_ARRAY[PID_COUNTER] = pcb;
			pthread_mutex_unlock(&MUTEX_PCB_ARRAY);

			return PID_COUNTER++;
		}

		// Se espera hasta que haya algún PID liberado para reutilizarlo
		while(LIST_RELEASED_PIDS->head == NULL)
			pthread_cond_wait(&COND_LIST_RELEASED_PIDS, &MUTEX_LIST_RELEASED_PIDS);

		t_link_element *element = LIST_RELEASED_PIDS->head;

		LIST_RELEASED_PIDS->head = element->next;
		LIST_RELEASED_PIDS->elements_count--;
	pthread_mutex_unlock(&MUTEX_LIST_RELEASED_PIDS);

	t_PID pid = (*(t_PID *) element->data);

	free(element->data);
	free(element);

	pthread_mutex_lock(&MUTEX_PCB_ARRAY);
		PCB_ARRAY[pid] = pcb;
	pthread_mutex_unlock(&MUTEX_PCB_ARRAY);

	return pid;

}

void pid_release(t_PID pid) {
	pthread_mutex_lock(&MUTEX_PCB_ARRAY);
		PCB_ARRAY[pid] = NULL;
	pthread_mutex_unlock(&MUTEX_PCB_ARRAY);

	t_link_element *element = malloc(sizeof(t_link_element));
		if(element == NULL) {
			log_error(MODULE_LOGGER, "No se pudo reservar memoria para el elemento de la lista de PID libres");
			exit(EXIT_FAILURE);
		}

		element->data = malloc(sizeof(t_PID));
		if(element->data == NULL) {
			log_error(MODULE_LOGGER, "No se pudo reservar memoria para el PID");
			exit(EXIT_FAILURE);
		}
		(*(t_PID *) element->data) = pid;

		pthread_mutex_lock(&MUTEX_LIST_RELEASED_PIDS);
			element->next = LIST_RELEASED_PIDS->head;
			LIST_RELEASED_PIDS->head = element;
		pthread_mutex_unlock(&MUTEX_LIST_RELEASED_PIDS);

		pthread_cond_signal(&COND_LIST_RELEASED_PIDS);
}

bool pcb_matches_pid(t_PCB *pcb, t_PID *pid) {
	return pcb->PID == *pid;
}

void log_state_list(t_log *logger, const char *state_name, t_list *pcb_list) {
	char *pid_string = string_new();
	pcb_list_to_pid_string(pcb_list, &pid_string);
	log_info(logger, "%s: [%s]", state_name, pid_string);
	free(pid_string);
}

void pcb_list_to_pid_string(t_list *pcb_list, char **destination) {
	if(destination == NULL || *destination == NULL || pcb_list == NULL)
		return;

	t_link_element *element = pcb_list->head;

	if(**destination && element != NULL)
		string_append(destination, ", ");

	char *pid_as_string;
	while(element != NULL) {
        pid_as_string = string_from_format("%" PRIu32, ((t_PCB *) element->data)->PID);
        string_append(destination, pid_as_string);
        free(pid_as_string);
		element = element->next;
        if(element != NULL)
            string_append(destination, ", ");
    }
}