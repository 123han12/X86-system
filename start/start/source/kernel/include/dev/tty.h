#ifndef TTY_H 
#define TTY_H
#include "dev/dev.h"
#include  "ipc/sem.h"

#define TTY_OBUF_SIZE     512 
#define TTY_IBUF_SIZE     512 

#define TTY_NR            8 

#define TTY_OCRLF        (1 << 0)        // 此位置1表示需要将在输出\n的时候需要对其转换为 \r\n

typedef struct _tty_fifo_t {
    char* buf ; 
    int size ; 
    int read , write ; 
    int count ; 

} tty_fifo_t ; 

typedef struct _tty_t {
    char obuf[TTY_OBUF_SIZE] ; 
    tty_fifo_t ofifo ; // 输出缓存 
    sem_t osem ; // 输出信号量

    char ibuf[TTY_IBUF_SIZE] ; 
    tty_fifo_t ififo ; // 输入缓存
    sem_t isem ; 

    int console_idx ;  

    int oflags ; 

}tty_t ; 

int tty_fifo_put(tty_fifo_t* fifo , char c ) ; 
int tty_fifo_get(tty_fifo_t* fifo , char* c) ; 


#endif