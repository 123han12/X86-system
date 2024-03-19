#include "common/boot_info.h"
#include "init.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "os_cfg.h"
#include "tools/log.h" 
#include "core/task.h"
#include "common/cpu_instr.h"
#include "tools/list.h"
#include "ipc/sem.h"
#include "ipc/mutex.h" 
#include "core/memory.h"

static boot_info_t* init_boot_info ;
static sem_t sem ; 


void kernel_init(boot_info_t* boot_info )  
{   
    init_boot_info = boot_info ;
    cpu_init() ; 
    log_init() ; 
    
    memory_init(boot_info) ;  
    
    irq_init() ; 
    time_init() ;  // 启动定时器

    task_manager_init() ;  // 任务管理器初始化
}


void move_to_first_task(void){
    task_t* curr = task_current() ; 
    ASSERT(curr != 0 ) ;

    tss_t* tss = &(curr->tss) ; 

    __asm__ __volatile__ (
        "push %[ss]\n\t"
        "push %[esp]\n\t"
        "push %[eflags]\n\t"
        "push %[cs]\n\t"
        "push %[eip]\n\t"
        "iret\n\t"
        : 
        : [ss]"r"(tss->ss) , [esp]"r"(tss->esp) , [eflags]"r"(tss->eflags) , [cs]"r"(tss->cs) , [eip]"r"(tss->eip) 
        : 
    ) ; 
} 

void init_main()
{

    log_printf("kernel is runing.......") ; 
    log_printf("version: %s  name:%s" , OS_VERSION , "tiny os x86") ;  
    log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a') ; 

    task_first_init() ;  
    move_to_first_task() ; 

}