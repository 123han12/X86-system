#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/irq.h"
#include "cpu/mmu.h" 
#include "dev/console.h"

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


// 在内存管理器alloc中获取page_count 个页面，返回物理地址的指针 , 失败返回0，并且返回的物理地址是4KB对齐的。
static uint32_t addr_alloc_page(addr_alloc_t* alloc , int page_count )
{
    uint32_t addr = 0 ; 
    mutex_lock(&alloc->mutex) ; 
    int page_index = bitmap_alloc_nbits(&alloc->bitmap , 0 , page_count ) ; 
    if(page_index >= 0 ) { // 如果存在的话找到第一个分配的地址。
        addr = alloc->start + page_index * alloc->page_size ;  
    }else {
        mutex_unlock(&alloc->mutex) ; 
        return 0 ; 
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

// alloc 为1表示当vaddr 对应的pte表项不存在的时候就重新分配，为0则不分配 , 返回的是虚拟地址在二级页表中的槽地址
pte_t* find_pte(pde_t* page_dir , uint32_t vaddr , int alloc) 
{   
    // 到最后page_table代表的是二级页表的起始地址(物理地址)
    pte_t* page_table = (pte_t*)0 ; 

    // 虚拟地址vaddr 在 页目录表中对应的槽地址
    pde_t* pde = page_dir + pde_index(vaddr) ; 

    if(pde->present){
        page_table = (pte_t*)pde_paddr(pde) ;   
    }else {
        if(alloc == 1 ){
            uint32_t page_paddr =  addr_alloc_page(&paddr_alloc , 1 ) ; // 新分配的一个物理地址(也是虚拟地址),作为一个二级页表
            if(page_paddr == 0 ) return (pte_t*) 0 ; 

            // 将这个物理地址设置到对应的页目录表中的槽, 并且设置相应的4MB的属性
            pde->v = page_paddr | PDE_P | PDE_W | PDE_U ; 

            // 将这个物理地址给page_table 作为二级页表的起始地址。
            page_table = (pte_t*)page_paddr ; 

            // 对这个新分配的4KB大小的页表进行一个清空。
            kernel_memset(page_table , 0 , MEM_PAGE_SIZE ) ; 


        }else { 
            return (pte_t*)0 ; 
        }
    }
            // 返回的是虚拟地址在二级页表中的槽地址。
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

        // 设置虚拟地址在二级页表中的4KB属性信息
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
       {(void*)CONSOLE_DISP_ADDR , (void*)CONSOLE_DISP_END , (void*)CONSOLE_DISP_ADDR , PTE_W } , 
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

// 创建一个一级页目录表，并且操作系统部分已经做好映射了。
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


// 在指定的一级页表page_dir中，给指定的vaddr虚拟地址开始分配up2(size)个物理页，并且属性为 perm ,失败返回-1
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
        if(err < 0 ) 
        {
            log_printf("create memory map is failed.....") ; 
            addr_free_page(&paddr_alloc , vaddr , i + 1 ) ; 
            return -1 ; 
        }
        
        curr_vaddr += MEM_PAGE_SIZE ; 
    }
    return 0 ; 
}

// 在当前进程的页表机制中，给指定的虚拟首地址addr开始分配up2(size) / MEM_PAGE_SIZE 个物理页面，每一个页面的属性都是perm
// 并且addr 和size 可以都不是页对齐的，addr会进行一个向下页对齐，size会进行一个向上页对齐。
int memory_alloc_page_for(uint32_t addr , uint32_t size , int perm ) 
{
    // 给指定页表的指定的虚拟地址分配指定的内存页的个数
    return memory_alloc_for_page_dir(task_current()->tss.cr3 , addr , size , perm ) ; 
}



// 返回从内存管理器中申请的物理地址,因为在1~128M内存都是线性映射，所以addr是物理地址也是其对应的虚拟地址,失败返回0
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


// 将指定页表中的内容复制一份，注意这个页表地址是物理地址 , 这里返回的是新创建的页目录表的地址，并且已经完成了复制。
uint32_t memory_copy_vum(uint32_t page_dir){
    uint32_t to_page_dir = memory_create_uvm() ;
    if(to_page_dir == 0){
        goto copy_uvm_failed ; 
    }

    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE) ; 
    pde_t* pde = (pde_t*)page_dir + user_pde_start ; 
    for(int i = user_pde_start ; i < PDE_CNT ; i ++ , pde ++ ){
        if(!pde->present ) continue ; 

        // pte 指向的是每一个二级页表的起始地址
        pte_t* pte = (pte_t*)pde_paddr(pde) ;
        for(int j = 0 ; j < PTE_CNT ; j ++ , pte ++ ) {
            if(!pte->present) continue ; // 二级页表中的当前槽位不存在值
            uint32_t page = addr_alloc_page(&paddr_alloc , 1 ) ; // 分配一个物理页
            if(page == 0 ){ // 错误处理机制
                goto copy_uvm_failed ; 
            }
            
            // 这里是按照页面进行拷贝的，所以vaddr 只需要找到对应的二级表项即可
            uint32_t vaddr = (i << 22 ) | (j << 12) ; 
            int err = memory_create_map((pde_t*)to_page_dir , vaddr , page , 1 , get_pte_perm(pte) ); 
            if(err < 0 ){
                goto copy_uvm_failed ; 
            } 
            kernel_memcpy((void*)page  , (void*)vaddr , MEM_PAGE_SIZE ) ; 
        }   
        
    }
    return to_page_dir ; 

copy_uvm_failed: 
    if(to_page_dir){
        memory_destroy_uvm(to_page_dir) ; 
    }
    return -1 ; 
}


// 释放给定的页目录表给定的内存。
void memory_destroy_uvm(uint32_t page_dir ) {
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE) ; 
    pde_t* pde = (pde_t*)page_dir + user_pde_start ; 

    // 注意，释放的时候从user_pde_start这个槽开始，千万不要从第0个槽开始，因为操作系统的代码是共享的
    for(int i = user_pde_start ; i < PDE_CNT ; i ++ , pde ++ ){
        if(!pde->present ) continue ;

        pte_t* pte = (pte_t*)pde_paddr(pde) ;
        for(int j = 0 ; j < PTE_CNT ; j ++ , pte ++ ) {
            if(!pte->present) continue ; // 二级页表中的当前槽位不存在值

            addr_free_page(&paddr_alloc , pte_paddr(pte) , 1 ) ; 
        }   

        addr_free_page(&paddr_alloc , (uint32_t)(pde_paddr(pde)) , 1 ) ; 
    }

    addr_free_page(&paddr_alloc , page_dir , 1 );
}


/// @brief 返回在指定的page_dir页表中 虚拟地址vaddr对应的物理地址
/// @param  page_dir 
/// @param  vaddr 
/// @return  
uint32_t  memory_get_paddr( uint32_t  page_dir , uint32_t  vaddr ) {

    pte_t* pte = find_pte((pde_t*)page_dir , vaddr , 0 ) ; 

    if(pte == (pte_t*)0 ){
        return 0 ; 
    }

    return pte_paddr(pte) + (vaddr & (MEM_PAGE_SIZE - 1) ) ;  
}


// 从当前正在使用的页表的from虚拟地址开始，拷贝size个字节到page_dir的to 这个虚拟地址
// to 这个地址可能不是4KB对齐的，需要注意一下
int memory_copy_uvm_data(uint32_t to , uint32_t page_dir , uint32_t from , uint32_t size ){
    while(size > 0 ){
        uint32_t to_paddr = memory_get_paddr(page_dir , to) ; 
        if(to_paddr == 0 ) {
            return -1 ; 
        }
        // 获取to的数据在当前页的偏移量
        uint32_t offset_in_page = to_paddr & (MEM_PAGE_SIZE - 1 ) ; 
        
        // curr_size 表示的是to在当前页应该拷贝的数据的字节数
        uint32_t curr_size = MEM_PAGE_SIZE - offset_in_page ; 

        // 获取到真实拷贝的字节数
        if(curr_size > size ){
            curr_size = size ; 
        } 
        kernel_memcpy((void*)to_paddr , (void*)from , curr_size) ; 
        
        size -= curr_size ; 
        to += curr_size ; 
        from += curr_size ; 
    }

    return 0 ; 
}


// 系统最终会调用到这个函数，来从堆空间中申请指定大小的内存，返回申请的内存的起始地址
char* sys_sbrk(int incr) {
    int bytes_count = incr ; 
    task_t* task = task_current() ; 
    char* pre_heap_end = (char*)task->heap_end ; 

    ASSERT(incr >= 0 ) ; 

    if(incr == 0 ) {
        log_printf("sbrk(0): end=0x%x" , pre_heap_end ); 
        return pre_heap_end ; 
    }    

    uint32_t start = task->heap_end  ; 

    uint32_t end = start + incr ; 

    int start_offset = start & (MEM_PAGE_SIZE - 1 ) ;  // 得到start 在一页中的偏移量
    if(start_offset) {
        if(start_offset + incr <= MEM_PAGE_SIZE ) {
            task->heap_end = end ; 
            return pre_heap_end ; 
        } else {
            uint32_t curr_size = MEM_PAGE_SIZE - start_offset ; 
            start += curr_size ; 
            incr -= curr_size ; 
        }
    }

    if(incr) {
        uint32_t curr_size = end - start ; 
        int err = memory_alloc_page_for(start , curr_size , PTE_P | PTE_U  | PTE_W ) ; 
        if(err < 0) {
            log_printf("sbrk: alloc memory failed...") ; 
            return (char*)-1 ;
        }
    }

    log_printf("sbrk(%d) , end = 0x%x" , bytes_count , end ) ; 
    task->heap_end = end ; 

    return (char*)pre_heap_end ;  
}