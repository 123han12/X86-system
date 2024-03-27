#include "dev/tty.h"
#include "dev/kbd.h"
#include "dev/console.h"
#include "cpu/irq.h"


static tty_t   tty_devs[TTY_NR] ; 

static int curr_tty = 0 ; 


/// @brief 用来初始指定的缓冲区管理结构 fifo 
/// @param fifo 
/// @param buf 
/// @param size 
static void tty_fifo_init(tty_fifo_t * fifo , char* buf , int size ) {
    fifo->buf = buf ; 
    fifo->size = size ; 
    fifo->read = fifo->write = 0 ; 
    fifo->count = 0 ; 
}

/// @brief 根据传入的dev中的idx 次设备号 minor 找到在tty_devs中的具体的显存区域,成功返回0，失败返回-1
/// @param dev 
/// @return 
int tty_open(device_t *dev) {
    int idx = dev->minor ;  // idx 表示的是在tty_devs中的下标
    if((idx < 0 ) || (idx >= TTY_NR) ) {
        log_printf("open tty failed , tty num = %d" , idx ) ; 
        return -1 ; 
    }
    tty_t* tty = tty_devs + idx ;   // 找到具体的tty_t 的结构

    /*
        每一个tty_t 结构都对应着一个console控制台，在tty_t中，每一个都有一个两个字符数组，
        相对于cpu来说，一个是输入缓冲队列，一个是输出缓冲队列。两个队列分别对应两个信号量，
        用于限制生产者和消费者对缓冲区的操作。

        console_idx 用来表示当前tty_t 在console_buf数组中的下标，一个槽对应的是一个显存块。

        oflags 表示的是当前的设备的一些属性 TTY_OCRLF 表示的是当前屏幕在遇到\n的时候输出的实际是\r\n
    

    */

    tty_fifo_init(&tty->ofifo , tty->obuf , TTY_OBUF_SIZE ) ;
    sem_init(&tty->osem , TTY_OBUF_SIZE ); // 初始值为 TTY_OBUF_SIZE  
    
    sem_init(&tty->isem , 0 ) ; // 键盘中断程序负责notify  
    tty_fifo_init(&tty->ififo , tty->ibuf , TTY_IBUF_SIZE ) ;
    tty->console_idx = idx ;  

    tty->oflags = TTY_OCRLF ;  
    tty->iflags = TTY_ICRLF | TTY_IECHO ; 



    // 初始化键盘
    kbd_init() ;  

    // 初始化指定的显存区域
    console_init(idx) ; 

    return 0 ; 
}


/// @brief 根据指定的device_t中的 dev->minor 找到指定tty_* 并返回
/// @param dev 
/// @return 
static tty_t* get_tty(device_t* dev ) {
    int idx = dev->minor ; 
    if((idx < 0 ) || (idx >= TTY_NR ) || (!dev->open_count ) ) {
        log_printf("tty is not opened..") ; 
        return (tty_t*) 0 ; 
    }

    return tty_devs + idx ;  
}



/// @brief 向fifo所管理的缓冲区中写入单个字符，写入成功返回0失败返回-1，调用方使用semphore来控制是否有空闲的位置
/// @param fifo 
/// @param c 
/// @return 
int tty_fifo_put(tty_fifo_t* fifo , char c ) {
    irq_state_t state = irq_enter_protection() ; 
    if(fifo->count == fifo->size ) {
        irq_exit_protection(state) ; 
        return -1 ; 
    }

    fifo->buf[fifo->write++] = c ; 
    if(fifo->write >= fifo->size ) fifo->write = 0 ; 
    fifo->count ++ ; 

    irq_exit_protection(state) ; 
    return 0 ; 
}

/// @brief 从指定的fifo所管理的缓冲区中读取单个字符，成功返回0，失败返回-1, 调用方使用semphore控制是否有字符
/// @param fifo 
/// @param c 
/// @return 
int tty_fifo_get(tty_fifo_t* fifo , char* c) {
    irq_state_t state = irq_enter_protection() ; 
    if(fifo->count == 0 ) {
        irq_exit_protection(state) ; 
        return -1 ; 
    }
    *c = fifo->buf[fifo->read++] ; 
    if(fifo->read >= fifo->size ) fifo->read = 0 ;
    fifo->count -- ; 
    irq_exit_protection(state) ; 
    
    return  0 ; 
}


/// @brief 向指定的tty设备中写入size个字符 , 返回写入的个数len
/// @param dev 
/// @param addr 
/// @param buf 
/// @param size 
/// @return 
int tty_write(device_t *dev, int addr, char *buf, int size){
    if(size < 0 ) {
        return -1 ; 
    }
    tty_t* tty = get_tty(dev) ; 

    if(!tty ) {
        return -1 ; 
    }

    int len = 0 ; 
    while(size) {
        char c = *buf ++ ; 

        if(c == '\n' && (tty->oflags & TTY_OCRLF )) {
            sem_wait(&tty->osem) ; 
            int err = tty_fifo_put(&tty->ofifo , '\r' ) ; 
            if(err < 0 ) break ;  
        }

        sem_wait(&tty->osem) ; 
        int err = tty_fifo_put(&tty->ofifo , c ) ; 
        if(err < 0 ) {
            break ; 
        }
        len ++ ; 
        size -- ; 
        console_write(tty); 
    }

    return len ; 
}


/// @brief 将tty从键盘中断函数中获取的字符输入到buf缓冲区中，一共输入size个
/// @param dev 
/// @param addr 
/// @param buf 
/// @param size 
/// @return 
int tty_read(device_t *dev, int addr, char *buf, int size){
    if(size < 0 ) {
        return -1 ;
    }
    tty_t* tty = get_tty(dev) ; 
    char* pbuf = buf ; 
    int len = 0 ; 
    while(len < size ) {
        sem_wait(&tty->isem) ; 
        char ch ; 
        tty_fifo_get(&tty->ififo , &ch ) ; 
        switch (ch)
        {
        case '\n':
            if((tty->iflags & TTY_ICRLF ) && (len < size - 1 ) ) {
                *pbuf++ = '\r' ;
                len ++;  
            }
            *pbuf++ = '\n' ; 
            len ++ ; 
            break ; 
        default:
            *pbuf++ = ch ; 
            len ++ ; 
            break;
        }

        //如果输入开启了回显,这个判断是为了让键盘输入的字符能够在屏幕上显示出来
        if(tty->iflags & TTY_IECHO) {
            tty_write(dev , addr , &ch , 1 ) ; 
        }   
        
        if(ch == '\n' || ch == '\r' ) {
            break ; 
        }
    }
    return len ; 

}



int tty_control(device_t *dev, int cmd, int arg0, int agr1){
    return 0 ; 
}
int tty_close(device_t *dev){

}


// 这个是在一开始就写死的，用来向操作系统注册tty设备的描述符
dev_desc_t dev_tty_desc = {
    .name = "tty", 
    .major = DEV_TTY , 
    .open = tty_open , 
    .read = tty_read , 
    .write = tty_write , 
    .control = tty_control , 
    .close = tty_close , 
} ; 

void tty_in( char ch ) {

    tty_t* tty = tty_devs + curr_tty ; 
    
    if(sem_count(&tty->isem) >= TTY_IBUF_SIZE ) {
        return ; 
    }
    tty_fifo_put(&tty->ififo , ch ) ; 
    sem_notify(&tty->isem) ; 
}

void tty_select(int tty) {
    if(tty != curr_tty ) {


        console_select(tty);  
        curr_tty = tty ; 

    }
}

