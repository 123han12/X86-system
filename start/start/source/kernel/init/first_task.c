#include "tools/log.h"
#include "core/task.h"
#include "applib/lib_syscall.h"


int first_task_main(void)
{
    int pid = getpid() ; 


    for(;;)
    {       
        print_msg("the process pid is:%d" , pid) ;  
        msleep(1000) ; 
    }   
}