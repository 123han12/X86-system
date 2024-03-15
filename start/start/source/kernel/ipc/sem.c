
#include "ipc/sem.h"
#include  "core/task.h" 
#include "cpu/irq.h" 
void sem_init(sem_t* sem , int init_count )
{
    sem->count = init_count ; 
    list_init( &sem->wait_list) ; 
}

void sem_wait(sem_t * sem) 
{
    irq_state_t state = irq_enter_protection() ; 
    if(sem->count >  0 ) 
    {
        sem->count -- ; 
    }else {
        task_t* curr = task_current() ; 
        task_set_block(curr) ; 
        list_insert_last( &sem->wait_list , &curr->wait_node ) ; 
       
        task_dispatch() ; // 进行任务的调度操作
    }
    irq_exit_protection(state) ; 
} 



void sem_notify(sem_t* sem  ) 
{
    irq_state_t state = irq_enter_protection() ; 

    if(list_count( &sem->wait_list) != 0 )
    {
        list_node_t* node = list_remove_first(&sem->wait_list) ; 
        task_t* task = list_parent_node(node , task_t , wait_node ) ; 
        task_set_ready(task) ; 
        task_dispatch() ; 
    }else {
        sem->count ++ ; 
    }

    irq_exit_protection(state) ; 
} 


int sem_count(sem_t* sem ) 
{
    irq_state_t state = irq_enter_protection() ; 
    int count = sem->count ; 
    irq_exit_protection(state) ;
    return count ;  
}
