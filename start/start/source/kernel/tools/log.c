#include "tools/log.h"
#include "common/cpu_instr.h"
#include "os_cfg.h"
#include <stdarg.h> 
#include "tools/klib.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "dev/console.h"
#include "dev/dev.h"


static mutex_t mutex ; 


static int log_dev_id ; 

// 这个宏用来控制是否使用串口，如果该宏值不为0表示使用串行接口，否则表示不使用
#define LOG_USE_COM    0  

#define COM1_PORT               0x3F8
void log_init(void)  // 设置qemu的串行接口的寄存器，硬件初始化，不用太在意
{
    mutex_init(&mutex) ; 

    // 实际上在这里就开启了第一个tty设备
    log_dev_id = dev_open(DEV_TTY , 0 , (void*)0 ) ; // 将log_dev_id   

#if LOG_USE_COM
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  
    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM1_PORT + 4, 0x0F);
#endif 
}

void log_printf(const char* fmt , ... )
{
    char str_buf[128] ;
    va_list args ;    // 这个东西是怎么用的
    
    kernel_memset(str_buf , '\0' , sizeof(str_buf) ) ; 
    
    va_start(args , fmt ) ;  

    kernel_vsprintf(str_buf , fmt , args) ; 
    va_end(args) ; 

    mutex_lock(&mutex) ; 

#if LOG_USE_COM
    const char * p = str_buf ; 

    
    while(*p != '\0')
    {
        while((inb(COM1_PORT + 5) & (1 << 6) ) == 0 ) ; 
        outb(COM1_PORT , *p++) ; 
    }
    outb(COM1_PORT , '\r') ; // 将光标移动到当前行的开头
    outb(COM1_PORT , '\n') ; // 将光标移动到当前行的结尾
#else 
    
    
    // console_write(0 , str_buf , kernel_strlen(str_buf) ) ; 

    dev_write(log_dev_id , 0 , "log:" , 4 ) ; 
    dev_write(log_dev_id , 0 , str_buf , kernel_strlen(str_buf ) ) ; 

    char c = '\n' ; 
    //console_write(0 , &c , 1 ) ;
    dev_write(log_dev_id , 0 , &c , 1 ) ;  
#endif  
    mutex_unlock(&mutex) ;
    
}


