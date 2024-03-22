#include "fs/fs.h"
#include "common/types.h"
#include "common/cpu_instr.h"
#include "common/boot_info.h"
#include "tools/klib.h"

#define TEMP_FILE_ID  100

static uint8_t TEMP_ADDR[100*1024] ; 
static uint8_t* temp_pos ; 

// 这个模块的代码实现的是从硬盘指定扇区开始，读取指定个数个扇区放到内存的指定物理地址的功能，不需太过纠结这个原理。
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

int sys_open(const char* name , int flags ){
    if(name[0] == '/'){ // 认为打开的是shell.elf文件
        read_disk(5000 , 80 , (uint8_t*) TEMP_ADDR ) ; // 读取elf文件到8M物理地址开始的地方
        temp_pos = (uint8_t*)TEMP_ADDR ; 
        return TEMP_FILE_ID ; 
    }

    return -1 ; 
}

// 将在TEMP_ADDR中的elf文件读入到指定的地址ptr 中, 
int sys_read(int file , char* ptr , int len ){
    if(TEMP_FILE_ID == file ) {
        kernel_memcpy((void*)ptr , (void*)temp_pos , len ) ; 
        temp_pos += len ; 
        return len ; 
    }
    return -1 ; 
}
int sys_write(int file , char* ptr , int len) {
    // 暂时不用，所以不写
    return -1 ; 
}
int sys_lseek(int file , int ptr , int dir ) {
    if(file == TEMP_FILE_ID ){
        temp_pos = (uint8_t*)(TEMP_ADDR + ptr ) ; 
        return 0 ; 
    }
    return -1 ; 
}

int sys_close(int file) {
    return 0 ; 
}