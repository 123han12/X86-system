#include "dev/tty.h"
#include "dev/kbd.h"
#include "dev/console.h"

static tty_t   tty_devs[TTY_NR] ; 


static void tty_fifo_init(tty_fifo_t * fifo , char* buf , int size ) {
    fifo->buf = buf ; 
    fifo->size = size ; 
    fifo->read = fifo->write = 0 ; 
    fifo->count = 0 ; 
}

int tty_open(device_t *dev) {
    int idx = dev->minor ;  // idx 表示的是在tty_devs中的下标
    if((idx < 0 ) || (idx >= TTY_NR) ) {
        log_printf("open tty failed , tty num = %d" , idx ) ; 
    }
    tty_t* tty = tty_devs + idx ; 

    tty_fifo_init(&tty->ofifo , tty->obuf , TTY_OBUF_SIZE ) ;
    sem_init(&tty->osem , TTY_OBUF_SIZE ); // 初始值为 TTY_OBUF_SIZE  
    tty_fifo_init(&tty->ififo , tty->ibuf , TTY_IBUF_SIZE ) ;
    tty->console_idx = idx ;  
    tty->oflags = TTY_OCRLF ;  

    kbd_init() ; 
    // 初始化指定的显存区域
    console_init(idx) ; 

    
    return 0 ; 
}

static tty_t* get_tty(device_t* dev ) {
    int idx = dev->minor ; 
    if((idx < 0 ) || (idx >= TTY_NR ) || (!dev->open_count ) ) {
        log_printf("tty is not opened..") ; 
        return (tty_t*) 0 ; 
    }

    return tty_devs + idx ;  

}


int tty_fifo_put(tty_fifo_t* fifo , char c ) {
    if(fifo->count == fifo->size ) {
        return -1 ; 
    }

    fifo->buf[fifo->write++] = c ; 
    if(fifo->write >= fifo->size ) fifo->write = 0 ; 
    fifo->count ++ ; 
    return 0 ; 
}
int tty_fifo_get(tty_fifo_t* fifo , char* c) {
    if(fifo->count == 0 ) {
        return -1 ; 
    }
    *c = fifo->buf[fifo->read++] ; 
    if(fifo->read >= fifo->size ) fifo->read = 0 ;
    fifo->count -- ; 
    return  0 ; 
}



int tty_read(device_t *dev, int addr, char *buf, int size){
    

}
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


}
int tty_control(device_t *dev, int cmd, int arg0, int agr1){
    return 0 ; 
}
int tty_close(device_t *dev){

}

dev_desc_t dev_tty_desc = {
    .name = "tty", 
    .major = DEV_TTY , 
    .open = tty_open , 
    .read = tty_read , 
    .write = tty_write , 
    .control = tty_control , 
    .close = tty_close , 
} ; 
