#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/irq.h"
#include "cpu/mmu.h" 

static addr_alloc_t paddr_alloc ; 


static pde_t kernel_page_dir[PDE_CNT]  __attribute__((aligned(MEM_PAGE_SIZE)) )  ; // 定义页目录表结构，并且令其起始地址4KB对齐  

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

// alloc 为1表示当vaddr 对应的pte表项不存在的时候就重新分配，为0则不分配
pte_t* find_pte(pde_t* page_dir , uint32_t vaddr , int alloc) 
{   
    pte_t* page_table = (pte_t*)0 ; 

    pde_t* pde = page_dir + pde_index(vaddr) ; 

    if(pde->present){
        page_table = (pte_t*)pde_paddr(pde) ;   
    }else {
        if(alloc == 1 ){
            uint32_t page_paddr =  addr_alloc_page(&paddr_alloc , 1 ) ; 
            if(page_paddr == 0 ) return (pte_t*) 0 ; 
            
            pde->v = page_paddr | PDE_P | PDE_W | PDE_U ; 

            page_table = (pte_t*)page_paddr ; 

            // 对这个新分配的4KB大小的页表进行一个清空。
            kernel_memset(page_table , 0 , MEM_PAGE_SIZE ) ; 


        }else { 
            return (pte_t*)0 ; 
        }
    }

    return page_table + pte_index(vaddr) ;  
}


// 在page_dir这个一级页表中建立vaddr和paddr之间的映射关系，count表示一共建立多少个页的映射关系，perm 表示每一个页的属性设置
// 成功返回0失败返回-1
int memory_create_map(pde_t* page_dir  , uint32_t vaddr  , uint32_t paddr  , int count ,  uint32_t perm  ) 
{
    for(int i = 0 ; i < count ; i ++ ) 
    {
        pte_t* pte = find_pte(page_dir , vaddr , 1 ) ; 
        if(pte== (pte_t *)0 ) {
            return -1 ; 
        }  
        
        // log_printf("find pte:0x%x" , pte ) ;

        ASSERT(pte->present == 0 ) ; 
        pte->v = paddr | perm | PTE_P ; 

        vaddr += MEM_PAGE_SIZE ; 
        paddr += MEM_PAGE_SIZE ; 

    }

    return 0 ; 
}



void  create_kernel_table(void) 
{
    extern uint8_t s_text[] , e_text[] , s_data[]  ;  
    static memory_map_t kernel_map[] = {
        {0 , s_text , 0 , PTE_W }  ,  
       {s_text , e_text , s_text ,  0 }  , 
       {s_data , (void*)MEM_EBDA_START , s_data , PTE_W  } , 

       {(void*)MEME_EXT_START , (void*)MEM_EXT_END , (void*)MEME_EXT_START, PTE_W } ,  
    } ; 

    for(int i = 0 ; i < sizeof(kernel_map) / sizeof(memory_map_t) ; ++i )
    {
        memory_map_t* map = kernel_map + i ; 
        
        uint32_t vstart = down2((uint32_t)map->vstart , MEM_PAGE_SIZE ) ; 
        uint32_t vend = up2((uint32_t)map->vend , MEM_PAGE_SIZE) ; 
        uint32_t paddr = down2((uint32_t)map->p_start , MEM_PAGE_SIZE ) ; 

        int page_count = (vend - vstart) / MEM_PAGE_SIZE ;  
        
        memory_create_map(kernel_page_dir , vstart , paddr , page_count ,  map->perm ) ;  

    }

}

void memory_init(boot_info_t* boot_info) 
{
    extern uint8_t* mem_free_start ; 
    uint8_t* mem_free = (uint8_t*)&mem_free_start ; 


    // show_memory_info(boot_info) ; 

    uint32_t mem_up1MB_free = total_memory_size(boot_info) - MEME_EXT_START ;  
    
    // 将能够分配给进程的物理内存的大小变为MEM_PAGE_SIZE 的整数倍，向下取整
    mem_up1MB_free = down2(mem_up1MB_free , MEM_PAGE_SIZE ) ; 



    // 对地址分配器paddr_alloc ，进行初始化 mem_free 表示位图缓存的起始地址，实际上其位于 .bss和MEM_EBDA_START 之间
    // 使用paddr_alloc 管理 mem_up1MB_free大小的空间，并且开始地址是 MEME_EXT_START 这个地方
    addr_alloc_init(&paddr_alloc ,mem_free ,  MEME_EXT_START , mem_up1MB_free , MEM_PAGE_SIZE) ; 
    
    // 获取在位图之后的空闲地址的开头地址

    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE ) ; 

    ASSERT(mem_free < (uint8_t*)MEM_EBDA_START ) ; 


    create_kernel_table() ; 
    mmu_set_page_dir((uint32_t)kernel_page_dir ); 


}

uint32_t memory_create_uvm(void) 
{
    // page_dir 是物理地址
    pde_t* page_dir = (pde_t*) addr_alloc_page(&paddr_alloc , 1 ) ; 
    if(page_dir == 0 ) 
    {
        return 0 ; 
    }

    kernel_memset(page_dir , 0 , MEM_PAGE_SIZE) ; 

    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE) ; 
    
    // 对于用户进程来说，在内核态下，不同的进程指向的是同一份物理二级页表，也就是同一份操作系统代码
    for(int i = 0 ; i < user_pde_start ; i ++ ) {
        page_dir[i].v = kernel_page_dir[i].v ; 
    }   

    return (uint32_t ) page_dir; 

}


int memory_alloc_for_page_dir(uint32_t page_dir , uint32_t vaddr , uint32_t size , uint32_t perm) 
{  
    uint32_t curr_vaddr = vaddr ; 
    int page_count = up2(size , MEM_PAGE_SIZE) / MEM_PAGE_SIZE ; 
    vaddr = down2(vaddr , MEM_PAGE_SIZE); 
    for(int i = 0 ; i < page_count; i ++ ) 
    {
        uint32_t paddr = addr_alloc_page(&paddr_alloc , 1 ) ; 
        if(paddr == 0 )
        {
            log_printf("memory alloc is failed.....") ; 
            return 0 ; 
        }

        int err = memory_create_map((pde_t*)page_dir , curr_vaddr , paddr , 1 , perm ) ; 
        if(err == -1 ) 
        {
            log_printf("create memory map is failed.....") ; 
            addr_free_page(&paddr_alloc , vaddr , i + 1 ) ; 
            return -1 ; 
        }
        
        curr_vaddr += MEM_PAGE_SIZE ; 
    }
    return 0 ; 
}

int memory_alloc_page_for(uint32_t addr , uint32_t size , int perm ) 
{
    // 给指定页表的指定的虚拟地址分配指定的内存页的个数
    return memory_alloc_for_page_dir(task_current()->tss.cr3 , addr , size , perm ) ; 
}



// 返回从内存管理器中申请的物理地址,因为在1~128M内存都是线性映射，所以addr是物理地址也是其对应的虚拟地址
uint32_t memory_alloc_page() {
    uint32_t addr = addr_alloc_page(&paddr_alloc , 1 ) ;    
    return addr ; 
}


static pde_t* curr_page_dir()
{
    return (pde_t*) (task_current())->tss.cr3 ; 
}

void memory_free_page(uint32_t addr )
{
    if(addr < MEMORY_TASK_BASE ){
        addr_free_page(&paddr_alloc , addr , 1 ) ; 
    }else {
        pte_t* pte = find_pte(curr_page_dir() , addr , 0 ) ; 
        
        ASSERT(pte == (pte_t*)0 && pte->present ) ;  

        addr_free_page(&paddr_alloc , pte_paddr(pte) , 1 ) ;
        
        pte->v = 0 ;  
    }
} 