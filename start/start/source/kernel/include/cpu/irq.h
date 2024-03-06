#ifndef IRQ_H
#define IRQ_H

#define IDT_TABLE_NR 128
#include "common/types.h"


typedef struct _exception_frame_t {
    uint32_t gs , fs , es , ds ; 
    uint32_t edi , esi , ebp , esp , ebx , edx , ecx , eax ; 
    uint32_t  num , error_code ;  
    uint32_t eip , cs , eflags ; 

} exception_frame_t ; 

typedef void(*irq_handler_t)(void) ; 

void irq_init(void) ; 
void exception_handler_unkown(void) ; 
void exception_handler_divider(void) ; 
uint32_t irq_install(uint32_t irq_num, irq_handler_t handler);

#endif 