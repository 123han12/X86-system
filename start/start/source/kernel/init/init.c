
#include "common/boot_info.h"
#include "init.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "os_cfg.h"
#include "tools/log.h" 
#include "core/task.h"
#include "common/cpu_instr.h"
#include "tools/list.h"


static boot_info_t* init_boot_info ; 

void kernel_init(boot_info_t* boot_info )  
{   
    init_boot_info = boot_info ; 
    log_init() ; 
    cpu_init() ; 
    irq_init() ; 
    time_init() ;  // 启动定时器

}

static task_t init_task ; 
static task_t first_task ; 
static uint32_t init_task_stack[1024] ; 

void init_task_entry(void)
{
    int count = 0 ; 
    for( ; ; ) { 

        log_printf("int task: %d" , count ++ ) ; 
        task_switch_from_to(&init_task , &first_task) ; 
    }
}

void list_test(void)
{
    list_t list ; 

    list_init(&list) ; 
    log_printf("list: first=0x%x , last=0x%x , count=%d" 
    , list_first(&list) , list_last(&list) , list_count(&list)
    ) ;

    list_node_t node1 , node2 , node3 , node4 ; 
    list_insert_first(&list , &node1) ; 
    list_insert_last(&list , &node2) ; 
    list_insert_first(&list , &node3) ;  // first            node3  node1   node2  node4 
    list_insert_last(&list , &node4) ;  // last 


    list_node_t * node5 =  list_remove(&list , &node3) ;
    list_node_t * node6 = list_remove(&list , &node2 ); 

    struct type_t{
        int a ; 
        list_node_t node ; 
    } v = {0x123456} ; 

    list_node_t* v_node = &v.node ; 

    struct type_t* addr_v = list_parent_node(v_node , struct type_t , node) ; 

}

void init_main()
{
    list_test() ; 

    log_printf("kernel is runing.......") ; 
    log_printf("version: %s  name:%s" , OS_VERSION , "tiny os x86") ;  
    log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a') ; 
   
    
    task_init(&init_task , (uint32_t)init_task_entry , (uint32_t)&init_task_stack[1024] ) ; 
    task_init(&first_task , 0 , 0 ) ;
    write_tr(first_task.tss_sel) ; 


    int count = 0 ; 
    for( ; ; ) { 
        log_printf("int main: %d" , count ++ ) ; 
        task_switch_from_to(&first_task , &init_task) ; 
    }

}