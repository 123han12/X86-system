// 汇编指令的封装

#ifndef CPU_INSTR_H 
#define CPU_INSTR_H 
#include "types.h"


// 从指定的端口中读入一个字节的数据返回
static inline uint8_t inb(uint16_t port )   
{
    uint8_t rv ; 
    __asm__  __volatile__ 
    (
        "inb %[p] , %[v] \n\t" 
        : [v]"=a"(rv)
        : [p]"d"(port)
    ) ; 
    return rv ; 
}

static inline uint16_t inw(uint16_t port )   
{
    uint16_t rv ; 
    __asm__  __volatile__ 
    (
        "inw %[p] , %[v] \n\t" 
        : [v]"=a"(rv)
        : [p]"d"(port)
    ) ; 
    return rv ; 
}


// 向指定端口输出数据
static inline void outb(uint16_t port , uint8_t data )
{   
    __asm__ __volatile__ (
        "outb %[v] , %[p]\n\t"
        : 
        : [p]"d"(port) , [v]"a"(data) 
    ) ; 
}   

static inline void cli(void)
{
    __asm__ __volatile__("cli") ; 
}

static inline void sti(void )
{
    __asm__ __volatile__("sti") ; 
}

static inline void lgdt(uint32_t start , uint32_t size ) 
{
    struct{
        uint16_t limit ; 
        uint16_t start15_0 ; 
        uint16_t start31_16 ; 
    } gdt ; 
    
    gdt.start31_16 = start >> 16 ; 
    gdt.start15_0 = start & 0xFFFF ; 
    gdt.limit = size - 1 ; 

    __asm__ __volatile__ (
        "lgdt %[g]"
        :
        :[g]"m"(gdt) 
    ) ; 
}

static inline uint32_t read_cr0() 
{
    uint32_t cr0 ; 
    __asm__ __volatile__(
        "movl %%cr0 , %[p]\n\t"
        : [p]"=r"(cr0) 
        : 
        : 
    ) ; 
    return cr0 ; 
}

static inline uint32_t write_cr0(uint32_t v )
{
    __asm__ __volatile__(
        "movl %[v] , %%cr0 \n\t"
        :
        :[v]"r"(v) 
        :
    ) ; 
}

static inline void far_jump(uint32_t selector , uint32_t offset )
{
    uint32_t addr[] = {offset , selector } ; 

    __asm__ __volatile__(
        "ljmpl *(%[a]) \n\t"
        :
        : [a]"r"(addr) 
        :
    ) ; 
}



#endif