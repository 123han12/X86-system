#include "core/syscall.h"
#include "core/task.h"
#include "tools/log.h"


typedef int (*sys_handler_t)(uint32_t arg0 , uint32_t arg1 , uint32_t arg2 , uint32_t arg3) ; 


void sys_printmsg(char* fmt , int arg)
{
    log_printf(fmt , arg) ; 
}

static const  sys_handler_t sys_table[] = {
    [SYS_sleep] = (sys_handler_t)sys_sleep , 
    [SYS_getpid] = (sys_handler_t)sys_getpid , 
    [SYS_fork] = (sys_handler_t)sys_fork , 
    [SYS_printmsg] = (sys_handler_t)sys_printmsg , 
    [SYS_execve] = (sys_handler_t)sys_execve , 
} ; 

void do_handler_syscall(sys_call_frame_t* frame ){
    if(frame->func_id < sizeof(sys_table) / sizeof(sys_handler_t) ) 
    {
        sys_handler_t handler = sys_table[frame->func_id] ; 
        if(handler) {
            int ret = handler(frame->arg0 , frame->arg1 , frame->arg2 , frame->arg3 ) ; 
            frame->eax = ret ;  
            return ; 
        }
    }
    task_t* task = task_current() ; 
    log_printf("task: %s , Unkown syscall: %d" , task->name , frame->func_id ) ; 
    frame->eax = -1 ; 
}