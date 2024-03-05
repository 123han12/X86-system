#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "common/cpu_instr.h"
#include "os_cfg.h"

static gate_desc_t idt_table[IDT_TABLE_NR] ; 

void exception_handler_unkown(void) ; 
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
}

static void do_default_handler(const char* message )
{
    for(; ;){ }
}


void do_handler_unkown(void)
{
    do_default_handler("unkown exception........") ; 
}