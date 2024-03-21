#ifndef MEMORY_H 
#define MEMORY_H 
#include "tools/bitmap.h"
#include "common/types.h"
#include "ipc/mutex.h" 
#include "common/boot_info.h" 

#define MEME_EXT_START    (1024*1024)
#define MEM_PAGE_SIZE     (4 * 1024) 
#define MEM_EBDA_START    0x80000
#define MEMORY_TASK_BASE  0x80000000
#define MEM_EXT_END       (128*1024*1024) 




typedef  struct  _addr_alloc_t {

    bitmap_t bitmap ; 
    mutex_t mutex ; 

    uint32_t start ; 
    uint32_t size ; 
    uint32_t page_size ;  // 一页的大小是多少


} addr_alloc_t ; 


typedef struct _memory_map_t {
    
    // 线性地址的起始与结束空间
    void* vstart  ; 
    void* vend ;
    void* p_start ; 

    // 特权相关的属性
    uint32_t perm ; 

}memory_map_t ; 



void memory_init(boot_info_t* boot_info ) ; 
uint32_t memory_create_uvm(void) ; 

// 给当前进程的一级页表addr虚拟地址开始的size个页面分配物理页面，每页的属性都是perm
int memory_alloc_page_for(uint32_t addr , uint32_t size , int perm ) ; 


uint32_t memory_alloc_page(void) ; 
void memory_free_page(uint32_t addr ) ; 



uint32_t memory_copy_vum(uint32_t page_dir) ;


void memory_destroy_uvm(uint32_t page_dir ) ; 


#endif 