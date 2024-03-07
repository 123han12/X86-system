
#include "common/boot_info.h"
#include "init.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "os_cfg.h"
 
static boot_info_t* init_boot_info ; 

void kernel_init(boot_info_t* boot_info )  
{   
    init_boot_info = boot_info ; 
    cpu_init() ; 
    irq_init() ; 
    time_init() ;  // 启动定时器
}

void init_main()
{
   // int a = 3 / 0 ;
    // irq_enable_global() ; 
    
    for( ; ; ) { }
}