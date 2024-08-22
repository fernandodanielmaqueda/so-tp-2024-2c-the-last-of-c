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

extern t_PCB *SYSCALL_PCB;

int syscall_execute(t_Payload *syscall_instruction);
int wait_kernel_syscall(t_Payload *syscall_arguments);
int signal_kernel_syscall(t_Payload *syscall_arguments);
int io_gen_sleep_kernel_syscall(t_Payload *syscall_arguments);
int io_stdin_read_kernel_syscall(t_Payload *syscall_arguments);
int io_stdout_write_kernel_syscall(t_Payload *syscall_arguments);
int io_fs_create_kernel_syscall(t_Payload *syscall_arguments);
int io_fs_delete_kernel_syscall(t_Payload *syscall_arguments);
int io_fs_truncate_kernel_syscall(t_Payload *syscall_arguments);
int io_fs_write_kernel_syscall(t_Payload *syscall_arguments);
int io_fs_read_kernel_syscall(t_Payload *syscall_arguments);
int exit_kernel_syscall(t_Payload *syscall_arguments);
int process_io_syscall(t_Payload *syscall_arguments, e_CPU_OpCode syscall_opcode, e_IO_Type required_io_type);

#endif // KERNEL_SYSCALLS_H