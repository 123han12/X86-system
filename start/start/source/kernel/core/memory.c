#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/irq.h"

static addr_alloc_t paddr_alloc ; 


static void addr_alloc_init(addr_alloc_t* alloc , uint8_t* bits , uint32_t start , uint32_t size , uint32_t page_size ) 
{
    mutex_init(&alloc->mutex) ;
    alloc->size = size ; 
    alloc->start = start ; 
    alloc->page_size = page_size ; 
    bitmap_init(&alloc->bitmap , bits , alloc->size / page_size , 0 ) ; 
}


// 在内存管理器alloc中获取page_count 个页面，返回物理地址的指针。
static uint32_t addr_alloc_page(addr_alloc_t* alloc , int page_count )
{
    uint32_t addr = 0 ; 
    mutex_lock(&alloc->mutex) ; 
    int page_index = bitmap_alloc_nbits(&alloc->bitmap , 0 , page_count ) ; 
    if(page_index >= 0 ) { // 如果存在的话找到第一个分配的地址。
        addr = alloc->start + page_index * alloc->page_size ;  
    }
    mutex_unlock(&alloc->mutex) ; 
    return addr ; 
}

// 在内存管理器中，释放从addr开始的物理内存，一共释放page_count个页面的大小
static void addr_free_page(addr_alloc_t* alloc , uint32_t addr , int page_count )
{
    mutex_lock(&alloc->mutex) ; 
    
    int page_index = (addr - alloc->start) / alloc->page_size ; 
    bitmap_set_bit(&alloc->bitmap , page_index , page_count , 0 ) ;  

    mutex_unlock(&alloc->mutex) ; 
}

void show_memory_info(boot_info_t* boot_info) 
{
    log_printf("mem region: ") ; 
    for(int i = 0 ; i < boot_info->ram_region_count ; i ++ )
    {
        log_printf("[%d]  start:0x%x  size:0x%x" , i , boot_info->ram_region_cfg[i].start , boot_info->ram_region_cfg[i].size ) ; 
    }

    log_printf("\n\n") ; 

}


static uint32_t total_memory_size(boot_info_t* boot_info)
{
    uint32_t mem_size = 0 ; 
    for(int i = 0 ; i < boot_info->ram_region_count ; i ++ ) 
    {
        mem_size += boot_info->ram_region_cfg[i].size ; 
    }
    return mem_size ; 
}

void memory_init(boot_info_t* boot_info) 
{
    extern uint8_t* mem_free_start ; 
    uint8_t* mem_free = (uint8_t*)&mem_free_start ; 

    log_printf("mem init") ; 

    show_memory_info(boot_info) ; 

    uint32_t mem_up1MB_free = total_memory_size(boot_info) - MEME_EXT_START ;  
    mem_up1MB_free = down2(mem_up1MB_free , MEM_PAGE_SIZE ) ; 

    log_printf("free memory:0x%x , size:0x%x" , MEME_EXT_START , mem_up1MB_free ) ; 


    addr_alloc_init(&paddr_alloc ,mem_free ,  MEME_EXT_START , mem_up1MB_free , MEM_PAGE_SIZE) ; 
    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE ) ; 

    ASSERT(mem_free < (uint8_t*)MEM_EBDA_START ) ; 

}