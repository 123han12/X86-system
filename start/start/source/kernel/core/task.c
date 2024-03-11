#include "core/task.h"
#include "cpu/irq.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "cpu/cpu.h"

static int tss_init(task_t * task , uint32_t entry , uint32_t esp )
{
    int tss_sel = gdt_alloc_desc() ; 
    if(tss_sel == -1) 
    {
        log_printf("alloc tss failed......") ;
        return -1 ;
    } 

    segment_desc_set(tss_sel , (uint32_t)&task->tss , sizeof(tss_t) , 
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS
    ) ; 

    kernel_memset(&task->tss , 0 , sizeof(tss_t) ) ; 
    task->tss.eip = entry ; 
    task->tss.esp = task->tss.esp0 = esp ; 
    task->tss.ss = task->tss.ss0 = KERNEL_SELECTOR_DS ; 
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = KERNEL_SELECTOR_DS ; 
    task->tss.cs = KERNEL_SELECTOR_CS ; 
    task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF ;
   
    task->tss_sel = tss_sel ; 
    return 0 ; 
}

int task_init(task_t * task , uint32_t entry , uint32_t esp ) 
{
    ASSERT(task != (task_t *) 0 ) ; 

    tss_init(task , entry , esp ) ; 

    return 0 ; 
} 


void task_switch_from_to(task_t* from , task_t* to )
{
    swith_to_tss(to->tss_sel) ; 
}