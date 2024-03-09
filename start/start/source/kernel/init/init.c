
#include "common/boot_info.h"
#include "init.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "os_cfg.h"
#include "tools/log.h" 


static boot_info_t* init_boot_info ; 

void kernel_init(boot_info_t* boot_info )  
{   
    init_boot_info = boot_info ; 

    ASSERT(boot_info->ram_region_count != 0) ; 

    ASSERT(1 == 0 ) ; 

    log_init() ; 
    cpu_init() ; 
    irq_init() ; 
    time_init() ;  // 启动定时器
}

void init_main()
{
    log_printf("kernel is runing.......") ; 
    log_printf("version: %s  name:%s" , OS_VERSION , "tiny os x86") ;  
    log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a') ; 
   
   
    // int a = 3 / 0 ;
    // irq_enable_global() ; 

    int num = 0 ;

    for( ; ; ) { }
}