#ifndef MEMORY_H 
#define MEMORY_H 
#include "tools/bitmap.h"
#include "common/types.h"
#include "ipc/mutex.h" 
#include "common/boot_info.h" 

#define MEME_EXT_START    (1024*1024)
#define MEM_PAGE_SIZE     (4 * 1024) 
#define MEM_EBDA_START    0x80000


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



#endif 