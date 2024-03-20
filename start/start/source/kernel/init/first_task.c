#include "tools/log.h"
#include "core/task.h"
#include "applib/lib_syscall.h"


int first_task_main(void)
{
    for(;;)
    {       
        msleep(1000) ; 
    }   
}