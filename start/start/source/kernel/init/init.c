
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


static boot_info_t* init_boot_info ; 

void kernel_init(boot_info_t* boot_info )  
{   
    init_boot_info = boot_info ; 
    log_init() ; 
    cpu_init() ; 
    irq_init() ; 
    time_init() ;  // 启动定时器

    task_manager_init() ;  // 任务管理器初始化
}

static task_t init_task ; 
static uint32_t init_task_stack[1024] ; 

void init_task_entry(void)
{
    int count = 0 ; 
    for( ; ; ) { 
         log_printf("second task: %d" , count ++ ) ; 
         sys_sleep(1000) ; 
    }
}


void init_main()
{

    log_printf("kernel is runing.......") ; 
    log_printf("version: %s  name:%s" , OS_VERSION , "tiny os x86") ;  
    log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a') ; 


    task_first_init() ;  
    task_init(&init_task , "init_task" , (uint32_t)init_task_entry , (uint32_t)&init_task_stack[1024] ) ; 


    irq_enable_global() ; 
    int count = 0 ; 
    for( ; ; ) { 
        log_printf("first task: %d" , count ++ ) ; 
        sys_sleep(1500) ; 
    }

}