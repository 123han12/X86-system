#include "fs/file.h"
#include "ipc/mutex.h"
#include "tools/klib.h"

static file_t file_table[FILE_TABLE_SIZE] ; 
static mutex_t file_alloc_mutex ;  

/// @brief 初始化全局的可被所有进程访问的file_table表，和用来实现进程互斥的锁 file_alloc_mutex 
/// @param void 
void file_table_init(void) {
    mutex_init(&file_alloc_mutex) ; 
    kernel_memset(file_table , 0 , sizeof(file_table) ) ;

}

/// @brief 从全局的file_table表中，寻找一个空的槽，将file_t* 返回给当前调用这个file_alloc()的进程。
/// @param  void
/// @return file_t* 
file_t* file_alloc(void) {
    file_t* file = (file_t*)0 ; 
    mutex_lock(&file_alloc_mutex) ; 
    for(int i = 0 ; i < FILE_TABLE_SIZE ; i ++ ) {
        file_t* curr_file = file_table + i ; 
        if(curr_file->ref == 0 ) {
            kernel_memset(curr_file , 0 , sizeof(file_t) ) ; 
            curr_file->ref = 1 ;
            file = curr_file ; 
            break ; 
        }
    }
    mutex_unlock(&file_alloc_mutex) ;
    return file ;  
} 


/// @brief 根据给定的file_t*指针，释放全局的file_table中的指定的槽。
/// @param (file_t*)
void file_free(file_t* file){

    mutex_lock(&file_alloc_mutex) ; 

    if(file->ref != 0 ){
        file->ref -- ; 
    }
    mutex_unlock(&file_alloc_mutex) ; 

}