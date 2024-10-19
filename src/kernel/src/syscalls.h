#ifndef KERNEL_SYSCALLS_H
#define KERNEL_SYSCALLS_H

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
#include "utils/module.h"
#include "utils/socket.h"
#include "kernel.h"

typedef struct t_Syscall {
    char *name;
    int (*function) (t_Payload *);
} t_Syscall;

extern t_Syscall SYSCALLS[];

int syscall_execute(t_Payload *syscall_instruction);

int wait_kernel_syscall(t_Payload *syscall_arguments);
int signal_kernel_syscall(t_Payload *syscall_arguments);
int process_create_kernel_syscall(t_Payload *syscall_arguments);
int process_exit_kernel_syscall(t_Payload *syscall_arguments);
int thread_create_kernel_syscall(t_Payload *syscall_arguments);
int thread_join_kernel_syscall(t_Payload *syscall_arguments);
int thread_cancel_kernel_syscall(t_Payload *syscall_arguments);
int thread_exit_kernel_syscall(t_Payload *syscall_arguments);
int mutex_create_kernel_syscall(t_Payload *syscall_arguments);
int mutex_lock_kernel_syscall(t_Payload *syscall_arguments);
int mutex_unlock_kernel_syscall(t_Payload *syscall_arguments);
int dump_memory_kernel_syscall(t_Payload *syscall_arguments);
int io_kernel_syscall(t_Payload *syscall_arguments);

void kill_thread(t_TCB *tcb);

#endif // KERNEL_SYSCALLS_H