#ifndef KLIB_H
#define KLIB_H 

#include "common/types.h"
#include <stdarg.h> 


// 将size 转换为bound的整数倍
static inline uint32_t down2(uint32_t size , uint32_t bound ) 
{
    return size & ~(bound - 1) ; 
}

// 将size 转换为bound的整数倍 并且向上取整
static inline uint32_t up2(uint32_t size , uint32_t bound ) 
{
    return (size + bound - 1) & ~(bound - 1) ;    
}


void kernel_strcpy(char * dest , const char * src ) ; 
void kernel_strncpy(char * dest , const char * src ,  uint32_t size ) ; 
uint32_t kernel_strncmp(const char * s1 , const char * s2 , uint32_t size) ; 
uint32_t kernel_strlen(const char* str) ; 
void kernel_memcpy(void* dest , void * src , uint32_t size ) ; 
void kernel_memset(void * dest , uint8_t v , uint32_t size ) ; 
uint32_t kernel_memcmp(void *d1 , void * d2 , uint32_t size ) ; 
void kernel_vsprintf(char * buffer, const char * fmt, va_list args);
void kernel_itoa(char * buf, int num, int base) ; 
void kernel_sprintf(char * buffer, const char * fmt, ...) ; 

#endif 