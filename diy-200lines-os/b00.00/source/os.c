/**
 * 功能：32位代码，完成多任务的运行
 *
 *创建时间：2022年8月31日
 *作者：李述铜
 *联系邮箱: 527676163@qq.com
 *相关信息：此工程为《从0写x86 Linux操作系统》的前置课程，用于帮助预先建立对32位x86体系结构的理解。整体代码量不到200行（不算注释）
 *课程请见：https://study.163.com/course/introduction.htm?courseId=1212765805&_trace_c_p_k2_=0bdf1e7edda543a8b9a0ad73b5100990
 */
#include "os.h"


// 这些数据类型中都带有_t, _t 表示这些数据类型是通过typedef定义的，而不是新的数据类型 方便代码的维护。
// 比如，在C中没有bool型，于是在一个软件中，一个程序员使用int，一个程序员使用short，会比较混乱。最好用一个typedef来定义一个统一的bool


typedef unsigned char uint8_t ; 
typedef unsigned short int uint16_t ; 
typedef unsigned int  uint32_t ; 

void do_syscall(int func , char* str , char color ) 
{
    static int row = 1 ; 
    if(func == 2 ) 
    {
        unsigned short * dest = (unsigned short *)0xb8000 + row * 80 ; 
        while(*str != '\0' ) 
        {
            *dest = *str ; 
            *dest |= (color << 8 ) ; 
            ++ dest , ++ str ; 
            // *dest ++ = *str ++ | (color << 8 ) ; 

        } 
        row = (row >= 25 ) ? 0 : row + 1 ; 
        
        for(int i = 0 ; i < 0xFFFFFF ; i ++ ) ; 
    }
}

void sys_show(char* str , char color ) 
{
    const unsigned long  addr[] = {0 ,  SYSCALL_SEG } ;  
    __asm__ __volatile__("push %[color];push %[str];push %[id];lcalll *(%[a])"::
        [a]"r"(addr) , [color]"m"(color) , [str]"m"(str) , [id]"r"(2)  
    ) ;
}

#define PDE_P   (1<<0)  // 是否有效
#define PDE_W   (1<<1)  // 是否能读写
#define PDE_U   (1<<2)  // 是否能被低特权级的程序读取
#define PDE_PS  (1<<7)  // 当前位置1的话，段大小为4M，二级页表在此模式下无效
#define MAP_ADDR 0x80000000 
void task_0(void) ; 
void task_1(void) ; 

uint32_t task0_dpl3_stack[1024] , task0_dpl0_stack[1024] , task1_dpl3_stack[1024] , task1_dpl0_stack[1024]  ;  

 // 加上一个值主要为了让os给map_phy_buffer 进行一个默认初始化 ， 从而当页表项为0的时候表示相应的页表项没有进行映射
uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = {0x36} ; 
static uint32_t page_table[1024]  __attribute__((aligned(4096))) = { PDE_U } ; 
// 创建页表机制
uint32_t pg_dir[1024] __attribute__((aligned(4096)))  = 
{
    [0] = (0) | PDE_P | PDE_W | PDE_PS | PDE_U ,    // 手动设置逻辑地址 0 - 4M 与 物理地址 0 - 4M 之间的映射关系  

};   // pg_dir 做4kB的对齐


struct {
    uint16_t  limit_l , base_l , basehl_attr , base_limit  ; 
} task0_ldt_table[2]  __attribute__((aligned(8)))  = {
    [TASK_CODE_SEG / 8 ] = {0xffff , 0x0000 , 0xfa00 , 0x00cf } , 
    [TASK_DATA_SEG / 8 ] = {0xffff , 0x0000 , 0xf300, 0x00cf} ,  
} ; 


struct {
    uint16_t  limit_l , base_l , basehl_attr , base_limit  ; 
} task1_ldt_table[2]  __attribute__((aligned(8)))  = {
    [TASK_CODE_SEG / 8 ] = {0xffff , 0x0000 , 0xfa00 , 0x00cf } , 
    [TASK_DATA_SEG / 8 ] = {0xffff , 0x0000 , 0xf300, 0x00cf } ,  
} ; 


// 定义 IDT 表
struct {
    uint16_t offset_l , selector , attr , offset_h ; 
} idt_table[256] __attribute__ ((aligned(8))) ;
// 定义gdt表
struct {
    uint16_t  limit_l , base_l , basehl_attr , base_limit  ; 
} gdt_table[256]  __attribute__((aligned(8))) = { // 这里设置必须存放在8地址对齐的字节数，因为是要求，所以必须满足
    [KERNEL_CODE_SEG / 8 ] = {0xffff , 0x0000 , 0x9a00 , 0x00cf } ,  
    [KERNEL_DATA_SEG / 8 ] = {0xffff , 0x0000 , 0x9200 , 0x00cf } ,  

    [APP_CODE_SEG / 8 ] = {0xffff , 0x0000 , 0xfa00 , 0x00cf } , 
    [APP_DATA_SEG  / 8 ] = {0xffff , 0x0000 , 0xf300 , 0x00cf } , 
    [TASK0_TSS_SEG / 8 ] = {0x0068 , 0x0000 ,  0xe900 , 0x0000 } , 
    [TASK1_TSS_SEG / 8 ] =  {0x0068 , 0x0000 , 0xe900 , 0x0000 } , 
    [SYSCALL_SEG / 8 ] = {0x0000 , KERNEL_CODE_SEG , 0xec03 , 0x0000 } , 

    [TASK0_LDT_SEG / 8 ] = { sizeof(task0_ldt_table) - 1 , 0x0000 , 0xe200 , 0x00cf } , 
    [TASK1_LDT_SEG / 8 ] = {sizeof(task1_ldt_table) - 1 , 0x0000 , 0xe200 , 0x00cf } , 

} ;  // 这里需要对gdt表进行初始化  , gdt_table中未被初始化的表项会被初始化为 0






uint32_t task0_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,  (uint32_t)task0_dpl0_stack + 4*1024, KERNEL_DATA_SEG , /* 后边不用使用 */ 0x0, 0x0, 0x0, 0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pg_dir,  (uint32_t)task_0/*入口地址*/, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task0_dpl3_stack + 4*1024/* 栈 */, 0x1, 0x2, 0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    TASK_DATA_SEG, TASK_CODE_SEG, TASK_DATA_SEG, TASK_DATA_SEG, TASK_DATA_SEG, TASK_DATA_SEG, TASK0_LDT_SEG, 0x0,
} ; 





uint32_t task1_tss[] = {

 // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,  (uint32_t)task1_dpl0_stack + 4*1024, KERNEL_DATA_SEG , /* 后边不用使用 */ 0x0, 0x0, 0x0, 0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)pg_dir,  (uint32_t)task_1/*入口地址*/, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task1_dpl3_stack + 4*1024/* 栈 */, 0x1, 0x2, 0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    TASK_DATA_SEG, TASK_CODE_SEG, TASK_DATA_SEG, TASK_DATA_SEG, TASK_DATA_SEG, TASK_DATA_SEG, TASK1_LDT_SEG, 0x0,
} ; 



void task_0(void)
{
    char * str = "task a : 1234"; 
    uint8_t color = 0 ; 

    for(; ; ) 
    {
        sys_show(str , color ++ ) ;  
    }
}

void task_1(void)
{
    char * str = "task b : 5678" ; 
    uint8_t color = 0xff ; 
    for( ; ; )
    {
        sys_show(str , color -- ); 
    }
}

 void outb(uint8_t data , uint16_t port ) 
{
    __asm__ __volatile__("outb %[v] , %[p]"::[p]"d"(port) , [v]"a"(data) ) ; 
} 


void task_sched(void)
{
    static int task_tss = TASK0_TSS_SEG ; 
    task_tss = (task_tss == TASK0_TSS_SEG) ? TASK1_TSS_SEG : TASK0_TSS_SEG ;

    uint32_t addr[] = {0 , task_tss } ; 
    __asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr)) ; 
}

void time_int(void) ; 

void syscall_handler(void) ; 


void os_init(void )
{
    // 对8259进行初始化
    outb(0x11 , 0x20) ; 
    outb(0x11 , 0xA0) ; 
    
    outb(0x20 , 0x21) ;
    outb(0x28 , 0xA1) ; 

    outb(1 << 2 , 0x21 ) ; 
    outb(2 , 0xa1 ) ;   

    outb(0x1 , 0x21 ) ; // 告知主芯片与其连接的cpu是8086系列的
    outb(0x1 , 0xa1) ;  // 写icw4 , 告诉从芯片8086 
    outb(0xfe , 0x21) ;  // 仅接受0号引脚的信号
    outb(0xff , 0xa1 ) ; 

    int tmo = 1193180 / 10 ;   // 每 100ms 产生一次中断
    outb(0x36 , 0x43) ;        // 设置特定的模式使得8253的计数值能反复设置为给定的值
    outb((uint8_t)tmo , 0x40) ;  // 四字节的底八位
    outb(tmo >> 8 , 0x40 ) ;  // 四字节的高八位

    idt_table[0x20].offset_h = (uint32_t)time_int >> 16 ;  
    idt_table[0x20].offset_l = (uint32_t)time_int & 0xffff ; 
    idt_table[0x20].selector = KERNEL_CODE_SEG ; 
    idt_table[0x20].attr = 0x8E00;

    gdt_table[TASK0_TSS_SEG / 8].base_l = (uint16_t)(uint32_t)task0_tss ; 
    gdt_table[TASK1_TSS_SEG / 8 ].base_l = (uint16_t)(uint32_t)task1_tss ; 
    gdt_table[SYSCALL_SEG / 8 ].limit_l = (uint16_t)(uint32_t)syscall_handler ; 
    gdt_table[TASK0_LDT_SEG / 8 ].base_l = (uint16_t)(uint32_t)task0_ldt_table ; 
    gdt_table[TASK1_LDT_SEG / 8 ].base_l = (uint16_t)(uint32_t)task1_ldt_table ; 



    pg_dir[MAP_ADDR >> 22 ] = (uint32_t)page_table | PDE_P | PDE_W | PDE_U ; 
    page_table[(MAP_ADDR >> 12) & 0x3ff] = (uint32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U ; 


}


 












