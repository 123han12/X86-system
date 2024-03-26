#include "core/task.h"
#include "cpu/irq.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "cpu/cpu.h"
#include "common/cpu_instr.h"
#include "cpu/irq.h"
#include "cpu/mmu.h"
#include "core/memory.h"
#include "core/syscall.h"
#include "fs/fs.h"
#include "common/elf.h"

static uint32_t idle_task_stack[IDLE_TASK_SIZE] ; 
static task_manager_t task_manager;     // 任务管理器

// 应用进程task_struct 管理结构
static task_t task_table[TASK_NR] ; 
static mutex_t task_table_mutex ; 


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
    task->parent = (task_t*)0 ; 

    int error = tss_init(task , flag , entry , esp ) ; 
    if(error == -1 ){
        log_printf("task init is failed....") ; 
        return  error ; 
    }
 
    kernel_strncpy(task->name , name , TASK_NAME_SIZE) ; 
    task->state = TASK_CREATED ; 
    task->sleep_ticks = 0 ; 

    task->heap_start = 0 ; 
    task->heap_end = 0 ; 

    task->time_ticks = TASK_TIME_SLICE_DEFAULT ; 
    task->slice_ticks = task->time_ticks ; 

    // 对 task_t 结构中的list_node_t 进行初始化
    list_node_init(&task->run_node) ;
    list_node_init(&task->all_node) ;  
    list_node_init(&task->wait_node) ; 
    kernel_memset(&task->file_table , 0 , sizeof(file_t*) * TASK_OFILE_NR ) ; 

    // 进行临界区保护
    irq_state_t  state = irq_enter_protection() ; 
    

    list_insert_last(&task_manager.task_list , &task->all_node) ;   
    irq_exit_protection(state) ; 


    return 0 ; 
} 

// 将已经初始化好的task结构加入到task_manager管理器中
void task_start (task_t* task) {
    irq_state_t state = irq_enter_protection() ; 
    task_set_ready(task) ; 

    irq_exit_protection(state) ; 
}


void task_uninit(task_t* task){
    if(task->tss_sel ) {
        gdt_free_sel(task->tss_sel) ; 
    }

    if(task->tss.esp0){
        memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE ) ; 
    }

    if(task->tss.cr3 ){
        memory_destroy_uvm(task->tss.cr3) ; 
    }

    kernel_memset(task , 0 , sizeof task) ; 
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

// 对任务表进行初始化
    kernel_memset(task_table , 0 , sizeof (task_table) ) ; 
    mutex_init(&task_table_mutex) ; 


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
    
    task_start(&task_manager.idle_task ) ;  
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
    
    // 初始化堆结构
    task_manager.first_task.heap_start = (uint32_t)e_first_task ; 
    task_manager.first_task.heap_end = (uint32_t)e_first_task ; 

   
    task_manager.curr_task = &(task_manager.first_task) ;  

    // 将first_task进程的一级页表的地址放入到cr3寄存器中    
    mmu_set_page_dir(task_manager.first_task.tss.cr3) ;  

    memory_alloc_page_for(first_start , alloc_size , PTE_P | PTE_W | PTE_U ) ; 
    kernel_memcpy((void*)first_start , (void*)s_first_task , copy_size ) ;   

    write_tr(task_manager.first_task.tss_sel) ; // 将其选择子放入到tr 寄存器中

    // 将初始化好的任务放到task_manager中
    task_start(&(task_manager.first_task) ) ; 
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


static task_t* alloc_task(void){
    task_t* task = (task_t*)0 ;

    mutex_lock(&task_table_mutex) ; 
    for(int i = 0 ; i < TASK_NR ; i ++ ){
        task_t* curr = task_table + i ; 
        if(curr->name[0] == '\0'){
            task = curr ; 
            break ;  
        }
    }
    mutex_unlock(&task_table_mutex) ; 
    return task ; 
}

static void free_task(task_t* task){
    mutex_lock(&task_table_mutex) ; 
    task->name[0] = '\0' ; 
    mutex_unlock(&task_table_mutex) ; 
}



// 创建子进程的一个流程

int sys_fork(){
    
    task_t* parent_task = task_current() ; 

    task_t* child_task = alloc_task() ; 
    if(child_task == (task_t*)0 ) {
        goto fork_failed ; 
    }

    // 获取在内核栈中压入的一些参数的设置。
    sys_call_frame_t* frame =(sys_call_frame_t*) ( parent_task->tss.esp0 - sizeof(sys_call_frame_t) )  ; 

    // 注意，因为在子进程中重新设置了页表，这里的 frame->esp + sizeof(uint32_t) * SYSCALL_COUNT 指的就是对应页表中的栈了，两者是不同的
    int err = task_init(child_task , parent_task->name , 0 , frame->eip , frame->esp + sizeof(uint32_t) * SYSCALL_COUNT ) ;
    if(err < 0 ) {
        goto fork_failed ; 
    } 

    // 子进程的tss的设置,当子进程获取到运行资格之后，会将tss中的内容恢复到各个寄存器中。
    tss_t* tss = &child_task->tss ; 
    tss->eax =  0 ;   // 当子进程在恢复的时候，这个就是子进程的返回值 
    tss->ebx = frame->ebx ; 
    tss->edx = frame->edx ; 
    tss->ecx = frame->ecx ; 
    tss->esi = frame->esi ; 
    tss->edi = frame->edi ; 
    tss->ebp = frame->ebp ; 

    tss->cs = frame->cs ; 
    tss->ds = frame->ds ;
    tss->es = frame->es ; 
    tss->fs = frame->fs ;   
    tss->gs = frame->gs ; 
    tss->eflags = frame->eflags ; 


    child_task->parent = parent_task ; 

    // 子进程和父进程共用同一个页表

    if((tss->cr3 = memory_copy_vum(parent_task->tss.cr3) ) < 0 ){
        goto fork_failed;  
    }
    task_start(child_task ) ; 
    return  child_task->pid ;  
fork_failed:
    if(child_task){
        task_uninit(child_task) ; 
        free_task(child_task) ; 
    }

    return -1 ; 
}

/// @brief 从指定文件，根据指定的段头结构elf_phdr 结构，读取段到 page_dir指定的页表之中
/// @param file 
/// @param elf_phdr 
/// @param page_dir 
/// @return 
static int load_phdr(int file , Elf32_Phdr* phdr , uint32_t page_dir ) {
    
    ASSERT((phdr->p_vaddr & (MEM_PAGE_SIZE - 1)) == 0); 
    // gcc在编译链接elf文件的时候，会自动将p_vaddr调整到4kB对齐
    int err = memory_alloc_for_page_dir(page_dir , phdr->p_vaddr , phdr->p_memsz , PTE_P | PTE_U | PTE_W ) ;
    if(err < 0 ){
        log_printf("no memory") ; 
        return -1 ;
    }
    if((sys_lseek(file , phdr->p_offset , 0 ) ) < 0 ) {
        log_printf("read file failed..") ; 
        return -1 ; 
    }

/*
    // 为段分配所有的内存空间.后续操作如果失败，将在上层释放
    // 简单起见，设置成可写模式，也许可考虑根据phdr->flags设置成只读
    // 因为没有找到该值的详细定义，所以没有加上

*/
    uint32_t vaddr = phdr->p_vaddr ; 
    uint32_t size = phdr->p_filesz ; 

    // 注意, 这个vaddr 应该是针对page_dir 这个页表，但现在这个页表还未启用，所以不能直接使用memset
    while(size > 0 ){
        int curr_size = (size > MEM_PAGE_SIZE ) ? MEM_PAGE_SIZE : size ; 
        
        // 得到vaddr 在page_dir页表中对应的物理地址
        uint32_t paddr = memory_get_paddr(page_dir , vaddr ) ; 

        // sys_read底层调用的是memory_copy , 因为在建立页表的时候，物理地址范围内的所有物理地址都和相应的虚拟地址是直接映射的，所以
        // 这个paddr , 可以在当前进程的当前页表中使用，作为新页表page_dir的物理地址的直接使用。而这个物理地址同样关联着page_dir的某个
        // 虚拟地址。
        if(sys_read(file , (char*)paddr , curr_size ) < curr_size ) {
            log_printf("read file failed...") ; 
            return -1 ;
        }
        
        size -= curr_size ; 
        vaddr += curr_size ; 
    }

    return 0 ; 
}




// 把name指定的elf文件加载到page_dir指定的虚拟地址空间中，
static uint32_t load_elf_file(task_t* task , const char* name , uint32_t page_dir) {
    Elf32_Ehdr elf_hdr ; 
    Elf32_Phdr elf_phdr ; 

    int file = sys_open(name , 0) ; // 这个函数实现了将elf文件加载到内存的指定位置，后序的读写都是从内存中读的，暂时是这样。
    if(file < 0 ){
        log_printf("open file is failed. %s" , name ) ; 
        goto load_failed ; 
    } 

    int cnt = sys_read(file , (char*)&elf_hdr , sizeof(Elf32_Ehdr) ) ; 
    if(cnt < sizeof(Elf32_Ehdr) ) { 
        log_printf("elf hdr too small. size=%d" , cnt ) ; 
        goto load_failed ; 
    }

    // 判断读出的文件是否正确
    if((elf_hdr.e_ident[0] != 0x7F) || (elf_hdr.e_ident[1] != 'E') || 
        (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F')
    ) {
        log_printf("check elf ident failed.") ;
        goto load_failed ; 
    }
     
    // 必须是可执行文件和针对386处理器的类型，且有入口
    if ((elf_hdr.e_type != ET_EXEC) || (elf_hdr.e_machine != ET_386) || (elf_hdr.e_entry == 0)) {
        log_printf("check elf type or entry failed.");
        goto load_failed;
    }


    // 必须有程序头部
    if ((elf_hdr.e_phentsize == 0) || (elf_hdr.e_phoff == 0)) {
        log_printf("none programe header");
        goto load_failed;
    }


    uint32_t e_phoff = elf_hdr.e_phoff ; 
    for(int i = 0 ; i < elf_hdr.e_phnum ; i ++ , e_phoff += elf_hdr.e_phentsize ){
        if( sys_lseek(file , e_phoff , 0 ) < 0 ) {
            goto load_failed ; 
        }  
        cnt = sys_read(file , (char*)&elf_phdr , sizeof(Elf32_Phdr) ) ; 
        if(cnt < sizeof(Elf32_Phdr) ){
            log_printf("read file faild....") ; 
            goto load_failed ; 
        }

        // 对 programme headr 结构进行检查
        if((elf_phdr.p_type != PT_LOAD ) || (elf_phdr.p_vaddr < MEMORY_TASK_BASE ) ) {
            continue ; 
        }

        // 加载当前程序头。
        int err = load_phdr(file , &elf_phdr , page_dir ) ;  
        if(err < 0 ) {
            log_printf("load program faild...") ; 
            goto load_failed ; 
        }

        task->heap_start = elf_phdr.p_vaddr + elf_phdr.p_memsz ; 
        task->heap_end = elf_phdr.p_vaddr + elf_phdr.p_memsz ; 
    }

    sys_close(file) ; 
    return elf_hdr.e_entry ;  // 返回整个elf文件的入口地址

load_failed:
    if(file){
        sys_close(file) ;
    }


    return 0 ; 
}


// 将 argv字符串数组中的argc个字符串一一拷贝到page_dir页表中对应的虚拟地址to开始的地方
int copy_args(char* to , uint32_t page_dir , int argc , char** argv ) {
    task_args_t task_args ; 
    task_args.argc = argc ; 
    task_args.argv = (char**)( to + sizeof(task_args_t) ) ; 
    task_args.ret_addr = 0 ; 
    

    // 获取字符串应该拷贝到的起始地址
    char* dest_arg = to + sizeof(task_args_t) + sizeof(char*) * argc ; 
    char** dest_arg_tb = (char**)memory_get_paddr(page_dir , (uint32_t)(to + sizeof(task_args_t) ) ) ; 
    // 拷贝字符串
    for(int i = 0 ; i < argc ; i ++ ){
        char* from = argv[i] ; 
        int len = kernel_strlen(from) + 1 ; 
        int err = memory_copy_uvm_data((uint32_t)dest_arg , page_dir , (uint32_t)from , len ) ;  
        ASSERT(err >= 0 ) ; 

        dest_arg_tb[i] = dest_arg ;  // 注意 dest_arg_tb是物理地址，这里可以直接使用的！！ 

        dest_arg += len ; 
    }
    int err = memory_copy_uvm_data((uint32_t)to , page_dir , (uint32_t)&task_args , sizeof(task_args_t) );

    return err ; 
}


int sys_execve(char* name , char ** argv , char **env ) {
    task_t* task = task_current() ; 

    kernel_memcpy(task->name , get_file_name(name) , TASK_NAME_SIZE) ; 

    uint32_t old_page_dir = task->tss.cr3 ; 
    uint32_t new_page_dir = memory_create_uvm() ; 
    if(new_page_dir == 0 ) {
        goto exec_failed ; 
    }

    uint32_t entry = load_elf_file(task , name , new_page_dir ) ; // 加载elf文件并返回入口地址
    if(entry == 0 ){
        goto exec_failed ; 
    }

    // 新进程的栈分配空间与first_task 可能不同，所以也需要重新建立
    uint32_t stack_top =  MEM_TASK_STACK_TOP - MEM_TASK_ARG_SIZE ;  
    int err = memory_alloc_for_page_dir(
        new_page_dir , 
        MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE , 
        MEM_TASK_STACK_SIZE , 
        PTE_P | PTE_U | PTE_W 
        ) ; 
    if(err < 0) {
        goto exec_failed ; 
    }
    int argc = strings_count(argv) ; 

    err = copy_args((char*)stack_top  , new_page_dir , argc , argv ) ;   
    if(err < 0 ){
        goto exec_failed ; 
    }

    sys_call_frame_t * frame = (sys_call_frame_t*) (task->tss.esp0 - sizeof(sys_call_frame_t)) ; 

    frame->eip = entry ; 
    frame->eax = frame->ebx = frame->ecx = frame->edx = 0 ; 
    frame->esi = frame->edi = frame->ebp = 0 ; 
    frame->eflags = EFLAGS_IF | EFLAGS_DEFAULT ; 
    // 注意 cs 和 ss 段寄存器不需要修改，所有的应用进程使用的都是task_manager 中的 app_code_sel 和 app_data_sel 
    frame->esp = stack_top - sizeof(uint32_t) * SYSCALL_COUNT ; 

    // 更新当前进程的页表
    task->tss.cr3 = new_page_dir ; 
    mmu_set_page_dir(new_page_dir) ; 

    memory_destroy_uvm(old_page_dir) ; 


    return 0 ; 

exec_failed:
    if(new_page_dir != 0 ){
        task->tss.cr3 = old_page_dir ; 
        memory_destroy_uvm(old_page_dir) ; 
        memory_destroy_uvm(new_page_dir) ; 
    }

    return -1 ; 
}


/// @brief 在进程中的file_table表中给指定的file_t结构分配下标，成功返回下标，失败返回-1
/// @param file 
/// @return 
int task_alloc_fd(file_t* file ) {
    task_t* task = task_current() ; 

    for(int i = 0 ; i < TASK_OFILE_NR ; i ++ ) {
        file_t* p = task->file_table[i] ; 
        if(p ==(file_t*)0 ) {
            task->file_table[i] = file ; 
            return i ; 
        } 
    }
    return -1 ; 
} 

/// @brief 清除进程中的的file_table中的fd槽中的file_t 的指针
/// @param fd 
void task_remove_fd(int fd){
    if((fd >= 0 ) && (fd < TASK_OFILE_NR ) ) {
        task_current()->file_table[fd] = (file_t*)0 ; 
        return ; 
    }
}


/// @brief 获取进程的file_table的第fd个槽中的file_t的指针
/// @param fd 
/// @return 
file_t* task_file(int fd){
    if((fd >= 0 ) && (fd < TASK_OFILE_NR) ) {
        file_t* file = task_current()->file_table[fd] ; 
        return file ; 
    }
    return (file_t*)0 ; 
} 