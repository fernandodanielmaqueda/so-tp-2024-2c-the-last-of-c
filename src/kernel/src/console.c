#include "console.h"

/*
int kernel_command_start_process(int argc, char* argv[]) {

    char *filename;
    t_Return_Value flag_relative_path;

    switch(argc) {
        case 2:
            if(*(argv[1]) == '/')
                filename = argv[1] + 1;
            else {
                filename = argv[1];
            }

            log_trace(CONSOLE_LOGGER, "INICIAR_PROCESO %s", argv[1]);
            flag_relative_path = 1;

            break;
        
        case 3:
            if(strcmp(argv[1], "-a") != 0) {
                log_warning(CONSOLE_LOGGER, "%s: Opción no reconocida", argv[1]);
                return 1;
            }

            filename = argv[2];
            log_trace(CONSOLE_LOGGER, "INICIAR_PROCESO -a %s", argv[2]);
            flag_relative_path = 0;

            break;

        default:
            log_warning(CONSOLE_LOGGER, "Uso: INICIAR_PROCESO [-a] <PATH (EN MEMORIA)>");
            return 1;
    }

    t_PCB *pcb = pcb_create();

     
    if(send_process_create(pcb->exec_context.PID, filename, flag_relative_path, CONNECTION_MEMORY.fd_connection)) {
        // TODO
        exit(1);
    }
  

    t_Return_Value return_value;
    if(receive_return_value_with_expected_header(PROCESS_CREATE_HEADER, &return_value, CONNECTION_MEMORY.fd_connection)) {
        // TODO
		exit(1);
    }
    if(return_value) {
        log_warning(MODULE_LOGGER, "No se pudo INICIAR_PROCESO %s", argv[1]);
        // TODO
        return 1;
    }

    pthread_mutex_lock(&(SHARED_LIST_NEW.mutex));
        list_add(SHARED_LIST_NEW.list, pcb);
    pthread_mutex_unlock(&(SHARED_LIST_NEW.mutex));

   // log_info(MINIMAL_LOGGER, "Se crea el proceso <%d> en NEW", pcb->exec_context.PID);

    sem_post(&SEM_LONG_TERM_SCHEDULER_NEW);

    return 0;
}
*/