
#include "common/boot_info.h"
#include "init.h"
#include "cpu/cpu.h"


static boot_info_t* init_boot_info ; 

void kernel_init(boot_info_t* boot_info )  
{   
    init_boot_info = boot_info ; 
    cpu_init() ; 
}

void init_main()
{
    for( ; ; ) { }
}