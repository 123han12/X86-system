#include "cpu/cpu.h"
#include "os_cfg.h"
#include "common/cpu_instr.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "core/syscall.h"
static segment_desc_t gdt_table[GDT_TABLE_SIZE] ; 

static mutex_t mutex ; 


// 设置指定selector选择子的属性，基地址base 段限长limit  段属性attr 
void segment_desc_set( uint16_t selector , uint32_t base , uint32_t limit , uint16_t attr ) 
{

    // 根据提供的信息设置当前的GDT 表项

    if(limit > 0xFFFFF ) 
    {
        limit >>= 12 ; 
        attr |= SEG_G ;  
    }
    segment_desc_t * desc = gdt_table + (selector >> 3 ); 
    desc->limit15_0 = limit & 0xFFFF ; 
    desc->base15_0 = base & 0xFFFF ; 
    desc->base23_16 = (base >> 16 ) & 0xFF ; 
    desc->attr = attr |  ( (( limit >> 16 ) & 0xF) << 8 ) ;   
    desc->base31_24 = (base >> 24 ) & 0xFF ; 
}


// 将 selector offset attr 设置到指定的调用门描述符地址desc指定的位置
void gate_dest_set(gate_desc_t* desc , uint16_t selector , uint32_t offset , uint16_t attr )
{
    desc->offset15_0 = offset & 0xFFFF ; 
    desc->selector = selector ; 
    desc->attr = attr ; 
    desc->offset31_16 = (offset >> 16) & 0xFFFF ; 
}

void init_gdt(void)
{
    for(int i = 0 ; i < GDT_TABLE_SIZE ; ++ i )
    {
        segment_desc_set(i << 3 , 0 , 0 , 0 ) ; 
    }

    // 设置代码段和数据段。
    segment_desc_set(KERNEL_SELECTOR_CS , 0x00000000 , 0xFFFFFFFF , 
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D | SEG_G 
    ) ;
    segment_desc_set(KERNEL_SELECTOR_DS , 0x00000000 , 0xFFFFFFFF ,  
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D | SEG_G 
    ) ;  
    

    // 在dgt表中设置调用门描述符，调用内核指定的exception_handler_syscall函数
    gate_dest_set(
        (gate_desc_t*)(gdt_table + (SELECTOR_SYSCALL >> 3 )) , KERNEL_SELECTOR_CS  , 
        (uint32_t)exception_handler_syscall , 
        GATE_P_PRESENT | GATE_DPL3 | GATE_TYPE_SYSCALL | SYSCALL_COUNT 
    ) ; 

    lgdt((uint32_t)gdt_table , sizeof(gdt_table) ) ;  // 将 gdt_table 表的起始地址放入到 gdtr 寄存器中

}

// 在gdt表中分配最小的段选择子槽给调用方，返回值是i * segment_desc_t , 分配不成功返回-1
int gdt_alloc_desc() 
{
    mutex_lock(&mutex) ; 
    for(int i = 1 ; i < GDT_TABLE_SIZE ; i ++ )
    {
        segment_desc_t * desc = gdt_table + i ; 
        if(desc->attr == 0 ) 
        { 
            mutex_unlock(&mutex) ; 
            return i * sizeof(segment_desc_t) ; 
        }
    }
    mutex_unlock(&mutex) ; 
    return -1 ; 
}

void swith_to_tss(int selector ) 
{
    far_jump(selector , 0 ) ;  // 实现程序切换的最重要的部分，就是跳转到指定的代码进行执行
}


void cpu_init(void)
{
    mutex_init(&mutex) ; 
    init_gdt() ; 
}

void gdt_free_sel(int sel ) 
{
    mutex_lock(&mutex) ; 

    gdt_table[sel / sizeof(segment_desc_t) ].attr = 0 ; 
    
    mutex_unlock(&mutex) ; 
}
