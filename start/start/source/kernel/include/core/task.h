#ifndef TASK_H
#define TASK_H
#include "cpu/cpu.h"
#include "common/types.h"
#include "tools/list.h"

#define TASK_NAME_SIZE    32 
#define TASK_TIME_SLICE_DEFAULT  10 



#define TASK_FLAGS_SYSTEM        (1 << 0)


typedef struct _task_t {

    enum {
        TASK_CREATED , 
        TASK_RUNNING , 
        TASK_SLEEP , 
        TASK_READY ,
        TASK_WAITTING ,  
    }state ; 

    int pid ; 

    struct _task_t* parent ;  // 记录当前进程的父进程。
    int sleep_ticks ; 
    int time_ticks ; 
    int slice_ticks ; 


    char name[TASK_NAME_SIZE] ; 

    list_node_t  run_node ; // 插入到  ready_list
    list_node_t wait_node ; 
    list_node_t  all_node ; // 插入到 task_list  
    
   
    tss_t tss ; 
    int tss_sel ; 

} task_t ; 


// 任务管理器的结构体的定义
typedef struct _task_manager_t {
    task_t * curr_task ; // 当前操作系统正在运行的任务

    list_t ready_list ; 
    list_t task_list ; 
    list_t sleep_list ;  // 延时队列
    

    task_t first_task ; 
    task_t idle_task ; 

    int app_code_sel ; 
    int app_data_sel ; 





} task_manager_t ; 


int task_init(task_t * task , const  char* name , int flag , uint32_t entry , uint32_t esp ) ; 

void task_switch_from_to(task_t* from , task_t* to ) ; 

void task_manager_init(void) ; 

void task_first_init(void) ;  

task_t* task_first_task(void) ; 

void task_set_ready(task_t* task) ; 

void task_set_block(task_t* task) ; 

int sys_sched_yield(void) ; 

task_t * task_current(void) ; 

void task_dispatch(void)  ; 

void task_time_tick(void) ; 



void task_set_sleep(task_t* task , uint32_t ticks) ; 

void task_set_wakeup(task_t* task) ; 

void sys_sleep(uint32_t ms) ;


int sys_getpid()  ; 

int sys_fork() ; 




#endif 