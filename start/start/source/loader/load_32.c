#include "loader.h"

// 这个模块的代码实现的是从硬盘指定扇区开始，读取指定个数个扇区放到内存的指定位置的功能，不需太过纠结这个原理。
static void read_disk(int sector , int sector_count , uint8_t* buf ) 
{
    outb(0x1F6, (uint8_t) (0xE0));

	outb(0x1F2, (uint8_t) (sector_count >> 8));
    outb(0x1F3, (uint8_t) (sector >> 24));		// LBA参数的24~31位
    outb(0x1F4, (uint8_t) (0));					// LBA参数的32~39位
    outb(0x1F5, (uint8_t) (0));					// LBA参数的40~47位

    outb(0x1F2, (uint8_t) (sector_count));
	outb(0x1F3, (uint8_t) (sector));			// LBA参数的0~7位
	outb(0x1F4, (uint8_t) (sector >> 8));		// LBA参数的8~15位
	outb(0x1F5, (uint8_t) (sector >> 16));		// LBA参数的16~23位

	outb(0x1F7, (uint8_t) 0x24) ; 

    uint16_t * data_buf = (uint16_t*) buf ; 
    while(sector_count -- ) 
    {
        while((inb(0x1F7) & 0x88 ) != 0x8) {}  // 检测当前扇区的数据是否准备好了。
        for(int i = 0 ; i < SECTOR_SIZE / 2 ; i ++ )
        {
            *data_buf++ = inw(0x1F0) ; 
        }
    }


}

// 这里没有开启分页机制，直接就是物理地址
void load_kernel(void)
{
    // 将 SYS_KERNEL_LOAD_ADDR 转换为函数指针，并且调用这个函数指针 就能跳入到内核进行执行了。
    read_disk(100 , 500 , (uint8_t *)SYS_KERNEL_LOAD_ADDR ) ;  

    ((void (*)(boot_info_t* ) )SYS_KERNEL_LOAD_ADDR)(&boot_info) ;    
    for( ;; ) {} 
}