#include "core/task.h"
#include "cpu/irq.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "cpu/cpu.h"
#include "common/cpu_instr.h"
#include "cpu/irq.h"
#include "cpu/mmu.h"
#include "core/memory.h"


static uint32_t idle_task_stack[IDLE_TASK_SIZE] ; 
static task_manager_t task_manager;     // 任务管理器

// 初始化指定task_struct 的tss段
static int tss_init(task_t * task , int flag , uint32_t entry , uint32_t esp )
{
    int tss_sel = gdt_alloc_desc() ; 
    if(tss_sel == -1) 
    {
        goto tss_init_failed ; 
    } 
    segment_desc_set(tss_sel , (uint32_t)&task->tss , sizeof(tss_t) , 
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS
    ) ; 

    kernel_memset(&task->tss , 0 , sizeof(tss_t) ) ; 



    uint32_t kernel_stack = memory_alloc_page() ; 

    if(kernel_stack == 0 ) 
    {
        goto tss_init_failed ; 
    }

    int code_sel , data_sel ; 
    if(flag & TASK_FLAGS_SYSTEM ) {
        code_sel = KERNEL_SELECTOR_CS ; 
        data_sel = KERNEL_SELECTOR_DS ; 
    }else {
        code_sel = task_manager.app_code_sel | SEG_CPL3 ; 
        data_sel = task_manager.app_data_sel | SEG_CPL3 ;  
    } 

    task->tss.eip = entry ; 
    task->tss.esp = esp ? esp : kernel_stack + MEM_PAGE_SIZE ; // 未指定栈则用内核栈，即运行在特权级0的进程 
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE ; 
    task->tss.ss = data_sel ; 
    task->tss.ss0 = KERNEL_SELECTOR_DS ; 
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = data_sel ; 
    task->tss.cs = code_sel ; 
    task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF ;
    task->tss.iomap = 0 ; 

    // 设置CR3字段，开启进程的自己的页表
    uint32_t page_dir = memory_create_uvm() ; // 从内存分配单元获取一个页目录表
    if(page_dir == 0 ) 
    {
        goto tss_init_failed ; 
    }
    task->tss.cr3 = page_dir ; 
   
    task->tss_sel = tss_sel ; 
    return 0 ; 
tss_init_failed:
    if(tss_sel > 0 ) 
    {
        gdt_free_sel(tss_sel) ; 
    }
    if(kernel_stack != 0 ) 
    {
        memory_free_page(kernel_stack) ; 
    }

    return -1 ; 
}

int task_init(task_t * task , const char * name , int flag ,  uint32_t entry , uint32_t esp ) 
{
    
    ASSERT(task != (task_t *) 0 ) ; 
    
    task->pid = (uint32_t)task ; 
    int error = tss_init(task , flag , entry , esp ) ; 
    if(error == -1 ){
        log_printf("task init is failed....") ; 
        return  error ; 
    }

    kernel_strncpy(task->name , name , TASK_NAME_SIZE) ; 
    task->state = TASK_CREATED ; 
    task->sleep_ticks = 0 ; 
    task->time_ticks = TASK_TIME_SLICE_DEFAULT ; 
    task->slice_ticks = task->time_ticks ; 

    // 对 task_t 结构中的list_node_t 进行初始化
    list_node_init(&task->run_node) ;
    list_node_init(&task->all_node) ;  
    list_node_init(&task->wait_node) ; 

    // 进行临界区保护
    irq_state_t  state = irq_enter_protection() ; 
    
    task_set_ready(task) ;  
    list_insert_last(&task_manager.task_list , &task->all_node) ;   

    irq_exit_protection(state) ; 


    return 0 ; 
} 

void simple_switch(uint32_t ** from , uint32_t * to ) ; 

void task_switch_from_to(task_t* from , task_t* to )
{
    swith_to_tss(to->tss_sel) ;
   // simple_switch( &from->stack , to->stack ) ; 
}


// 空闲进程执行的代码 
static void idle_task_entry()
{
    for( ;; ) {
        hlt() ; 
    } 
}

void task_manager_init(){

    int sel = gdt_alloc_desc() ;
    segment_desc_set(sel ,  0x00000000 , 0xFFFFFFFF , SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW 
       |  SEG_D 
    ) ; 
    task_manager.app_data_sel = sel ; 

    sel = gdt_alloc_desc() ;
    segment_desc_set(sel ,  0x00000000 , 0xFFFFFFFF , SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW 
       |  SEG_D 
    ) ; 
    task_manager.app_code_sel = sel ; 


    list_init(&(task_manager.ready_list) ) ; 
    list_init(&(task_manager.task_list) ) ; 
    list_init(&(task_manager.sleep_list) ) ; 
    task_manager.curr_task = (task_t*)0 ;  
    task_init(&task_manager.idle_task , "idle_task" , TASK_FLAGS_SYSTEM ,
     (uint32_t)idle_task_entry , 0);  // esp 等于0因为其运行在特权级为0所以无需指定特权级为3的栈

}


/**
 * @brief 初始进程的初始化
 * 没有采用从磁盘加载的方式，因为需要用到文件系统，并且最好是和kernel绑在一定，这样好加载
 * 当然，也可以采用将init的源文件和kernel的一起编译。此里要调整好kernel.lds，在其中
 * 将init加载地址设置成和内核一起的，运行地址设置成用户进程运行的高处。
 * 不过，考虑到init可能用到newlib库，如果与kernel合并编译，在lds中很难控制将newlib的
 * 代码与init进程的放在一起，有可能与kernel放在了一起。
 * 综上，最好是分离。
 */
void task_first_init(void)  
{   
    void first_task_entry(void) ;

    // s_first_task  和 e_first_task 分别是first_task的起始地址和结束地址(物理上的) 
    extern uint8_t s_first_task[] , e_first_task[] ;

    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task) ; 
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE ; // 分配10个物理页
    ASSERT(copy_size < alloc_size) ; 
    

    uint32_t first_start = (uint32_t) first_task_entry ;  

    task_init( &(task_manager.first_task) , "first_task"  , 0 , first_start , first_start + alloc_size ) ; 
   
    task_manager.curr_task = &(task_manager.first_task) ;  

    // 将first_task进程的一级页表的地址放入到cr3寄存器中    
    mmu_set_page_dir(task_manager.first_task.tss.cr3) ;  

    memory_alloc_page_for(first_start , alloc_size , PTE_P | PTE_W | PTE_U ) ; 
    kernel_memcpy((void*)first_start , (void*)s_first_task , copy_size ) ;   


    write_tr(task_manager.first_task.tss_sel) ; // 将其选择子放入到tr 寄存器中
}

task_t* task_first_task(void)  
{
    return &(task_manager.first_task) ; 
}

void task_set_ready(task_t* task)
{
    if(task == &task_manager.idle_task ) return ; 
    list_insert_last(&task_manager.ready_list , &task->run_node ) ; 
    task->state = TASK_READY ; 
}

void task_set_block(task_t* task)
{
    if(task == &task_manager.idle_task ) return ; 
    list_remove(&task_manager.ready_list , &task->run_node) ; 
}


task_t * task_current(void)
{
    return task_manager.curr_task ; 
}

int sys_sched_yield(void) 
{   
    // 进行临界区保护
    irq_state_t  state = irq_enter_protection() ; 

    if(list_count(&task_manager.ready_list) > 1 ) 
    {
        task_t* curr_task = task_current() ; 
        
        // 将read_list 的头结点移动到最后。
        task_set_block(curr_task) ; 
        task_set_ready(curr_task) ; 

        task_dispatch() ; 

    }

    irq_exit_protection(state) ; 

    return 0 ; 
}


static task_t * task_next_run(void)
{
    if(list_count(&task_manager.ready_list) == 0 ) return &task_manager.idle_task ; 

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

        // 这里的from 的 state 需要进行修改吗

        task_manager.curr_task = to ; 
        to->state = TASK_RUNNING ; 

        task_switch_from_to(from , to) ; 
    }
}

void task_time_tick(void) 
{


    task_t * curr_task = task_current() ; 

    irq_state_t state = irq_enter_protection() ; 

    if(--curr_task->slice_ticks == 0 ) 
    {
        // 设置下一次调度的时间
        curr_task->slice_ticks = curr_task->time_ticks ; 

        // 将read_list 的头结点移动到最后。
        task_set_block(curr_task) ; 
        task_set_ready(curr_task) ; 
        
        task_dispatch() ; 
    }

    list_node_t * curr = list_first(&task_manager.sleep_list) ; 
    list_node_t* end = curr ; 
    
    // 注意，如果你设置的list是双向循环链表的话，这里就不能简单的设置为curr 而应该
    if(curr) 
    {
        list_node_t* next = list_node_next(curr) ; 
        task_t* task = list_parent_node(curr , task_t , run_node) ; 
        if(--task->sleep_ticks == 0 ) 
        {
            task_set_wakeup(task) ; 
            task_set_ready(task) ;  
        }
        curr = next ; 
    }

    while(curr && curr != end)   
    {
        list_node_t* next = list_node_next(curr) ; 
        task_t* task = list_parent_node(curr , task_t , run_node) ; 
        if(--task->sleep_ticks == 0 ) 
        {
            task_set_wakeup(task) ; 
            task_set_ready(task) ;  
        }
        curr = next ; 
    }

    task_dispatch() ;

    irq_exit_protection(state) ;  
}



void task_set_sleep(task_t* task , uint32_t ticks)
{
    if(ticks <= 0 ) return ; 

    task->sleep_ticks = ticks ; 
    task->state = TASK_SLEEP ; 
    list_insert_last(&(task_manager.sleep_list) , &task->run_node ) ;


}
void task_set_wakeup(task_t* task) {
    list_remove((&task_manager.sleep_list) , &task->run_node ) ; 
} 

void sys_sleep(uint32_t ms) {
    if(ms < OS_TICK_MS) ms = OS_TICK_MS ;

    irq_state_t state = irq_enter_protection() ; 

    task_set_block(task_manager.curr_task) ; 
    task_set_sleep(task_manager.curr_task , ( ms + (OS_TICK_MS - 1 ) ) / OS_TICK_MS ) ;

    // 切换进程
    
    task_dispatch() ;  

    irq_exit_protection( state ) ; 
}


int sys_getpid() {
    task_t* task = task_current() ; 
    return task->pid ; 
}