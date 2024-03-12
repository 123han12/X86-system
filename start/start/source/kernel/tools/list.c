#include "tools/list.h"

void list_init(list_t* list)
{   
    list->first = list->last =  (list_node_t *)0 ; 
    list->count = 0 ;
}   

void list_insert_first(list_t *list , list_node_t* node )
{
    if(list_is_empty(list) == 0 )
    {
        list->first = node ; 
        list->last = node ; 
        node->next = node->pre = node ; 
        list->count ++ ; 
        return ; 
    }
    
    node->next = list->first ; 
    node->pre = list->first->pre ; 
    
    list->first->pre->next = node ; 
    list->first->pre = node ; 
    
    list->first = node ; 

    list->count++ ; 
}
void list_insert_last(list_t* list , list_node_t* node ) 
{
    if(list_is_empty(list) == 0 )
    {
        list->first = node ; 
        list->last = node ; 
        node->next = node->pre = node ; 
        list->count ++ ; 
        return ; 
    }
    node->next = list->last->next ; 
    node->pre = list->last ;  

    list->last->next->pre = node ; 
    list->last->next = node ; 

    list->last = node ; 
    list->count++ ; 
    return ; 
}



list_node_t*  list_remove_first(list_t* list ) 
{
    if(list_is_empty(list) == 0 ) 
    {
        return (list_node_t*)0 ; 
    }
    if(list_count(list) == 1 ) 
    {
        list_node_t* node = list->first ; 
        list_init(list) ; 
        node->pre = node->next = (list_node_t*)0 ; 
        return node ; 
    }

    list->first->pre->next = list->first->next ; 
    list->first->next->pre = list->first->pre ; 

    list_node_t* node = list->first ; 
    list->first = list->first->next ; 

    list->count-- ; 
    node->pre = node->next = (list_node_t*)0 ; 
    return node ; 
} 


list_node_t*  list_remove_last(list_t* list) 
{
    if(list_is_empty(list) == 0 ) 
    {
        return (list_node_t*)0 ; 
    }
    if(list_count(list) == 1 ) 
    {
        list_node_t* node = list->first ; 
        list_init(list) ; 
        node->pre = node->next = (list_node_t*)0 ; 
        return node ; 
    }
    list->last->pre->next = list->last->next ; 
    list->last->next->pre = list->last->pre ; 

    list_node_t* node = list->last ; 
    list->last = list->last->pre ; 

    list->count-- ; 

    node->pre = node->next = (list_node_t*)0 ; 
    return node ; 
}

list_node_t* list_remove(list_t* list , list_node_t* node )  // 假设传入的node一定在list链表中
{
    if(list->count == 1 && node == list->first )
    {
        list_init(list) ; 
        node->pre = node->next = (list_node_t*)0 ; 
        return node ; 
    }

    if(node == list->first ) return list_remove_first(list) ;
    if(node == list->last ) return list_remove_last(list) ; 

    node->pre->next = node->next ; 
    node->next->pre = node->pre ; 

    node->next = node->pre = (list_node_t* )0 ; 
    
    return node ; 
}

