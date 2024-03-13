#include "core/task.h"
#include "cpu/irq.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "cpu/cpu.h"
#include "common/cpu_instr.h"

static task_manager_t task_manager;     // 任务管理器

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

int task_init(task_t * task , const char * name ,  uint32_t entry , uint32_t esp ) 
{
    ASSERT(task != (task_t *) 0 ) ; 
    tss_init(task , entry , esp ) ; 


    kernel_strncpy(task->name , name , TASK_NAME_SIZE) ; 
    task->state = TASK_CREATED ; 
    task->time_ticks = TASK_TIME_SLICE_DEFAULT ; 
    task->slice_ticks = task->time_ticks ; 

    // 对 task_t 结构中的list_node_t 进行初始化
    list_node_init(&task->run_node) ;
    list_node_init(&task->all_node) ;  

    task_set_ready(task) ;  
    list_insert_last(&task_manager.task_list , &task->all_node) ;   

    return 0 ; 
} 

void simple_switch(uint32_t ** from , uint32_t * to ) ; 

void task_switch_from_to(task_t* from , task_t* to )
{
    swith_to_tss(to->tss_sel) ;
   // simple_switch( &from->stack , to->stack ) ; 
}




void task_manager_init(){

    list_init(&(task_manager.ready_list) ) ; 
    list_init(&(task_manager.task_list) ) ; 
    task_manager.curr_task = (task_t*)0 ; 

}

void task_first_init(void)  
{   
    task_init( &(task_manager.first_task) , "first_task"  , 0 , 0 ) ;
    write_tr(task_manager.first_task.tss_sel) ; // 将其选择子放入到tr 寄存器中、

    task_manager.curr_task = &(task_manager.first_task) ;  
}

task_t* task_first_task(void)  
{
    return &(task_manager.first_task) ; 
}

void task_set_ready(task_t* task)
{
    list_insert_last(&task_manager.ready_list , &task->run_node ) ; 
    task->state = TASK_READY ; 
}

void task_set_block(task_t* task)
{
    list_remove(&task_manager.ready_list , &task->run_node) ; 
}


task_t * task_current(void)
{
    return task_manager.curr_task ; 
}

int sys_sched_yield(void) 
{   
    if(list_count(&task_manager.ready_list) > 1 ) 
    {
        task_t* curr_task = task_current() ; 
        
        // 将read_list 的头结点移动到最后。
        task_set_block(curr_task) ; 
        task_set_ready(curr_task) ; 

        task_dispatch() ; 

    } 

    return 0 ; 
}


task_t * task_next_run(void)
{
    list_node_t* task_node = list_first(&task_manager.ready_list ) ; 

    task_t* task = list_parent_node( task_node , task_t  , run_node) ; 

    return task ; 
}

void task_dispatch(void) 
{
    task_t* to = task_next_run() ; 
    if(to != task_manager.curr_task ) 
    {
        task_t* from = task_current() ; 

        // 这里的from 的 state 需要进行修改吗？
        from->state = TASK_READY ; 

        task_manager.curr_task = to ; 
        to->state = TASK_RUNNING ; 

        task_switch_from_to(from , to) ; 
    }

}

void task_time_tick(void) 
{
    task_t * curr_task = task_current() ; 

    if(--curr_task->slice_ticks == 0 ) 
    {
        // 设置下一次调度的时间
        curr_task->slice_ticks = curr_task->time_ticks ; 

        // 将read_list 的头结点移动到最后。
        task_set_block(curr_task) ; 
        task_set_ready(curr_task) ; 
        

        // 进行任务切换
        task_dispatch() ; 
    }

}