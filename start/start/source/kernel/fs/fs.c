#include "fs/fs.h"
#include "common/types.h"
#include "common/cpu_instr.h"
#include "common/boot_info.h"
#include "tools/klib.h"
#include "tools/log.h" 
#include "dev/console.h"
#include "fs/file.h"
#include "dev/dev.h"
#include "core/task.h" 


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

static int is_path_vaild(const char* path ) {
    if(path == (const char*)0 || path[0] == '\0' ) {
        return 0 ; 
    }
    return 1 ; 
}

/// @brief 根据指定的路径打开文件，如果是设备文件，则调用dev_open函数 
/// @param name 
/// @param flags 
/// @return 
int sys_open(const char* name , int flags ){
    
    if(kernel_memcmp((void*)name , (void*)"tty" , 3 ) == 0 ) {
        if(!is_path_vaild(name) ) {
            log_printf("path is not vaild....") ; 
            return -1 ;
        }

        file_t* file = file_alloc() ;
        int fd = -1 ; 
        if(file) {
            fd = task_alloc_fd(file) ;
            if(fd < 0 ) {
                goto sys_open_failed; 
            }
        } else {
            goto sys_open_failed; 
        }

        // 针对于 tty:0 这种特殊的文件格式，后期会进行修改
        int num = name[4] - '0' ; 

        int dev_id = dev_open(DEV_TTY , num , 0 ) ; 
        if(dev_id < 0 ) {
            goto sys_open_failed ; 
        }

        file->dev_id = dev_id ;  // 设置当前file对应的dev_table中的下标
        file->mode = 0 ; 
        file->pos = 0 ; 
        file->ref = 1 ; 
        file->type = FILE_TTY ;   // 表明是一个设备文件
        kernel_strncpy(file->file_name , name , FILE_NAME_SIZE ) ; // 设置文件名称

        return fd ; 
sys_open_failed:
        if(file) {
            file_free(file) ; 
        }
        if(fd >= 0 ) {
            task_remove_fd(fd) ; 
        }
        return -1 ;

    } else if (name[0] == '/'){ // 认为打开的是shell.elf文件
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
    } else if(file == 0 ) {
        
    }
    return -1 ; 
}
int sys_write(int file , char* ptr , int len) {
    file = 0 ; 
    file_t* p_file = task_file(file) ; 
    if(!p_file) {
        log_printf("file not opened...") ; 
        return -1 ; 
    }

    return dev_write(p_file->dev_id , 0 , ptr , len ) ; 

    return -1; 

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


int sys_isatty(int file){
    return -1 ; 
}
int sys_fstat(int file , struct stat* st ) {
    return -1 ;
}

void fs_init(void) {
    file_table_init() ; 
}