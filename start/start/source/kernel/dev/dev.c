#include "dev/dev.h"
#include "cpu/irq.h"
#include "tools/klib.h"
#include "dev/disk.h"

#define DEV_TABLE_SIZE     128 

extern dev_desc_t dev_tty_desc ; 
extern dev_desc_t dev_disk_desc ; 

// 设备种类表，操作系统每接入一种新的设备，都需要在这里插入新的设备种类
// 并且每个设备种类元素都在自己的编译单元内设置好了自己的特定的处理函数
static dev_desc_t* dev_desc_table[] = {
    &dev_tty_desc , 
    &dev_disk_desc , 

} ; 

// 存放具体的物理设备的信息,每一个元素代表的都是一个真实的物理设备，
// 其中的dev_desc_t* 字段用来描述该物理设备的设备种类。
static device_t dev_table[DEV_TABLE_SIZE] ; 

// 判断给定的dev_id 是否有效
static int is_device_bad(int dev_id ){
    if(dev_id < 0 || dev_id >= DEV_TABLE_SIZE) {
        return 1 ; 
    }
    if(dev_table[dev_id].desc == (dev_desc_t*)0) {
        return 1 ; 
    }
    return 0 ; 

}

/// @brief 主设备号，次设备号 , 参数指针, 返回值是在dev_table中的下标 
/// @param major 
/// @param minor 
/// @param data 
/// @return 
int dev_open(int major , int minor , void *data ) {
    irq_state_t state = irq_enter_protection() ; 

    device_t* free_dev = (device_t*)0 ; 

    for(int i = 0 ; i < DEV_TABLE_SIZE ; i ++ ) {
        device_t* dev = dev_table + i ; 
        if(dev->open_count == 0 ) {
            free_dev = dev ;
        } else if(dev->desc->major == major && (dev->minor == minor ) ) {
            dev->open_count ++ ; 
            irq_exit_protection(state) ; 
            return i ; 
        } 
    }

    dev_desc_t * desc = (dev_desc_t*)0 ; 
    for(int i = 0 ; i < sizeof(dev_desc_table) / sizeof(dev_desc_table[0] ) ; i ++ ) {
        dev_desc_t* d = dev_desc_table[i] ; 
        if(d->major == major ) {
            desc = d ; 
            break ; 
        } 
    }


    // 如果走到这里，desc仍然为(dev_desc_t*)0 则表示该种类型的设备操作系统并未设置其具体的处理函数，直接忽略
    // 如果走到这里，free_dev为 (device_t*)0 表示此时物理设备槽满了，也直接不处理。

    if(desc && free_dev ) {
        free_dev->minor = minor ; 
        free_dev->data = data ; 
        free_dev->desc = desc ; 

        // 调用指定的设备的特定的open()函数，0表示成功 -1 表示失败
        int err = desc->open(free_dev) ; 
        if(err == 0 ) {      // 表示打开成功了
            free_dev->open_count = 1 ; 
            irq_exit_protection(state) ; 
            return free_dev - dev_table ; 
        }
    }
    irq_exit_protection(state) ; 
    return -1 ; 
}

// dev_id 表示的是在dev_table中的下标
int dev_read(int dev_id , int addr , char* buf , int size ) {
    if(is_device_bad(dev_id) ) {
        return -1 ; 
    }
    device_t* dev = dev_table + dev_id ; 
    return dev->desc->read(dev , addr , buf , size ) ; 
}

// 根据给定的dev_id,向指定的设备中写入数据
int dev_write(int dev_id , int addr , char* buf , int size) {
    if(is_device_bad(dev_id) ) {
        return -1 ; 
    }
    device_t* dev = dev_table + dev_id ; 
    return dev->desc->write(dev , addr , buf , size ) ; 
}


int dev_control(int dev_id , int cmd , int arg0 , int arg1 ) {
    if(is_device_bad(dev_id) ) {
        return -1 ; 
    }
    device_t* dev = dev_table + dev_id ; 
    return dev->desc->control(dev , cmd , arg0 , arg1 ) ; 
}


void dev_close(int dev_id){
    if(is_device_bad(dev_id) ) {
        return ; 
    }
    device_t* dev = dev_table + dev_id ; 
    irq_state_t state = irq_enter_protection() ; 
    if(--(dev->open_count) == 0 ) {
        dev->desc->close(dev) ; 
        kernel_memset(dev , 0 , sizeof(device_t ) ) ; 
    }
    irq_exit_protection(state) ; 
}
