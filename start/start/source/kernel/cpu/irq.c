#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "common/cpu_instr.h"
#include "os_cfg.h"

static gate_desc_t idt_table[IDT_TABLE_NR] ; 




static void do_default_handler(exception_frame_t* frame , const char* message )
{
    for(; ;){ }
}

void do_handler_unkown(exception_frame_t* frame )
{
    do_default_handler(frame , "unkown exception........") ; 
}

void do_handler_divider(exception_frame_t* frame)
{
    do_default_handler(frame , "divider exception........") ; 
}

uint32_t irq_install(uint32_t irq_num , irq_handler_t handler )  
{
    if(irq_num >= IDT_TABLE_NR ) 
    {
        return -1 ; 
    }
    gate_dest_set(idt_table + irq_num , KERNEL_SELECTOR_CS , (uint32_t)handler , 
      GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT   
    ) ; 
} 

void irq_init(void)
{
    // 初始化每一个表项
    for(int i = 0 ; i < IDT_TABLE_NR ; i ++ )
    {
        gate_dest_set(( idt_table + i ) , KERNEL_SELECTOR_CS , (uint32_t)exception_handler_unkown , 
            GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT ) ;  
    }

    
    // 将 idt_table 地址加载到 idtr 寄存器
    lidt((uint32_t)idt_table , sizeof(idt_table) ) ; 

    irq_install(0 , exception_handler_divider) ;  
}