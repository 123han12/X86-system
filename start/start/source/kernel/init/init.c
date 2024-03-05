
#include "common/boot_info.h"
#include "init.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"


static boot_info_t* init_boot_info ; 

void kernel_init(boot_info_t* boot_info )  
{   
    init_boot_info = boot_info ; 
    cpu_init() ; 
    irq_init() ; 
}

void init_main()
{
    int a = 3 / 0 ;  
    int b ; 
    for( ; ; ) { }
}