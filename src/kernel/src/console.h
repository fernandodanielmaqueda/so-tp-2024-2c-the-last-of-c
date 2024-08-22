#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include "commons/collections/dictionary.h"
#include "utils/arguments.h"
#include "utils/module.h"
#include "utils/socket.h"
#include "kernel.h"

#define MAX_CONSOLE_ARGC 1 + 2

extern t_log *CONSOLE_LOGGER;
extern char *CONSOLE_LOG_PATHNAME;

typedef struct t_Command {
    char *name;
    int (*function) (int, char*[]);
} t_Command;

extern t_Command CONSOLE_COMMANDS[];

extern int EXIT_CONSOLE;

extern int KILL_EXEC_PROCESS;
extern pthread_mutex_t MUTEX_KILL_EXEC_PROCESS;

extern unsigned int MULTIPROGRAMMING_LEVEL;
extern unsigned int CURRENT_MULTIPROGRAMMING_LEVEL;
extern pthread_mutex_t MUTEX_MULTIPROGRAMMING_LEVEL;
extern pthread_cond_t COND_MULTIPROGRAMMING_LEVEL;

extern int SCHEDULING_PAUSED;
extern pthread_mutex_t MUTEX_SCHEDULING_PAUSED;

void *initialize_kernel_console(void *argument);
void initialize_readline(void);
char **console_completion(const char *text, int start, int end);
char *command_generator(const char *text, int state);
char *strip_whitespaces(char *string);
int execute_line(char *line);
t_Command *find_command(char *name);
int kernel_command_run_script(int argc, char* argv[]);
int kernel_command_start_process(int argc, char* argv[]);
int kernel_command_kill_process(int argc, char* argv[]);
int kernel_command_pause_scheduling(int argc, char* argv[]);
int kernel_command_resume_scheduling(int argc, char* argv[]);
int kernel_command_multiprogramming(int argc, char* argv[]);
void wait_multiprogramming_level(void);
void signal_multiprogramming_level(void);
int kernel_command_process_states(int argc, char* argv[]);

#endif // KERNEL_CONSOLE_H