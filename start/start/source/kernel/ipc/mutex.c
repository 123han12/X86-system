
#include "ipc/mutex.h" 
#include "cpu/irq.h" 
#include "tools/list.h"

void mutex_init(mutex_t* mutex)
{
    mutex->locked_count = 0 ; 
    mutex->owner = (task_t*)0 ; 
    list_init(&mutex->wait_list) ; 
}


void mutex_lock(mutex_t* mutex) 
{
    irq_state_t state = irq_enter_protection() ; 
    task_t* current = task_current() ; 
    if(mutex->locked_count == 0 ) 
    {
        mutex->locked_count ++ ; 
        mutex->owner = current ; 
    }else if(mutex->owner == current ){
        mutex->locked_count ++ ; 
    }else{
        task_set_block(current) ; 
        list_insert_last(&mutex->wait_list , &current->wait_node) ; 
        task_dispatch() ; 
    }

    irq_exit_protection(state) ; 
}

void mutex_unlock(mutex_t* mutex)
{
    irq_state_t state = irq_enter_protection() ; 
    task_t* current = task_current() ; 
    if(mutex->owner == current)
    {
        if( -- mutex->locked_count == 0 )
        {
            mutex->owner = (task_t*) 0 ; 
            if(list_count(&mutex->wait_list)) 
            {
                list_node_t* node = list_remove_first(&mutex->wait_list) ;
                task_t* task = list_parent_node(node , task_t , wait_node) ; 
                task_set_ready(task) ; 

                mutex->locked_count = 1 ; 
                mutex->owner = task ; 

                task_dispatch() ; 
            }  
        } 
    }

    irq_exit_protection(state) ;
} 
