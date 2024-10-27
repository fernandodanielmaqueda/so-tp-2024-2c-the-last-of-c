/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef KERNEL_TRANSITIONS_H
#define KERNEL_TRANSITIONS_H

#include "kernel.h"

void kill_thread(t_TCB *tcb);

int get_state_new(t_PCB **pcb);
int get_state_ready(t_TCB **tcb);
int get_state_blocked_io_ready(t_TCB **tcb);
int get_state_blocked_io_exec(t_TCB **tcb);
int get_state_exit(t_TCB **tcb);

int locate_and_remove_state(t_TCB *tcb);

int insert_state_new(t_PCB *pcb);
int insert_state_ready(t_TCB *tcb, e_Process_State previous_state);
int insert_state_exec(t_TCB *tcb, e_Process_State previous_state);
int insert_state_blocked_join(t_TCB *tcb, t_TCB *target, e_Process_State previous_state);
int insert_state_blocked_io_ready(t_TCB *tcb, e_Process_State previous_state);
int insert_state_blocked_io_exec(t_TCB *tcb, e_Process_State previous_state);
int insert_state_exit(t_TCB *tcb, e_Process_State previous_state);

int reinsert_state_new(t_PCB *pcb);

#endif