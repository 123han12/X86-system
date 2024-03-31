#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "common/cpu_instr.h"
#include "common/boot_info.h"

static disk_t disk_buf[DISK_CNT] ; 


// 向磁盘的指定的分区发送命令
static void disk_send_cmd(disk_t* disk , uint32_t start_sector , uint32_t sector_count , int cmd ) {

    outb(DISK_DRIVE(disk), DISK_DRIVE_BASE | disk->drive);		// 使用LBA寻址，并设置驱动器

	// 必须先写高字节
	outb(DISK_SECTOR_COUNT(disk), (uint8_t) (sector_count >> 8));	// 扇区数高8位
	outb(DISK_LBA_LO(disk), (uint8_t) (start_sector >> 24));		// LBA参数的24~31位
	outb(DISK_LBA_MID(disk), 0);									// 高于32位不支持
	outb(DISK_LBA_HI(disk), 0);										// 高于32位不支持
	outb(DISK_SECTOR_COUNT(disk), (uint8_t) (sector_count));		// 扇区数量低8位
	outb(DISK_LBA_LO(disk), (uint8_t) (start_sector >> 0));			// LBA参数的0-7
	outb(DISK_LBA_MID(disk), (uint8_t) (start_sector >> 8));		// LBA参数的8-15位
	outb(DISK_LBA_HI(disk), (uint8_t) (start_sector >> 16));		// LBA参数的16-23位

	// 选择对应的主-从磁盘
	outb(DISK_CMD(disk), (uint8_t)cmd) ;
}

static void disk_read_data(disk_t* disk , void* buf , int size) {
    uint16_t* c = (uint16_t*)buf ; 

    for(int i = 0 ; i < size / 2 ; i ++ ) {
        *c++ = inw(DISK_DATA(disk) ) ; 
    }
}
static void disk_write_data(disk_t* disk , void* buf , int size ) {
    uint16_t* c = (uint16_t*)buf ; 
    for(int i = 0 ; i < size / 2 ; i ++ ) {
        outw(DISK_DATA(disk) , *c++) ; 
    }   
}

static int disk_wait_data(disk_t* disk ) {
    uint8_t status ; 
    do {
        status = inb(DISK_STATUS(disk) ) ; // 读取状态
        if( (status & (DISK_STATUS_BUSY | DISK_STATUS_DRQ | DISK_STATUS_ERR ) ) != DISK_STATUS_BUSY ) {
            break ; 
        } 
    }while(1) ; 

    return (status & DISK_STATUS_ERR ) ? -1 : 0 ; 

}

static void detect_part_info(disk_t* disk ) {
    mbr_t mbr ; 
    disk_send_cmd(disk , 0 , 1 , DISK_CMD_READ) ; 
    int err = disk_wait_data(disk) ; 
    if(err < 0 ) {
        log_printf("read mbr failed...") ; 
        return ; 
    }

    // 将MBR部分读入到mbr结构体中
    disk_read_data(disk , &mbr , sizeof(mbr) ) ;  

    part_item_t * item = mbr.part_item ; 
    partinfo_t* part_info = disk->partinfo + 1 ; 
    for(int i = 0 ; i < MBR_PRIMARY_PART_NR ; i ++ , item ++ , part_info++ ) {
        part_info->type = item->system_id ;
        if(part_info->type == FS_INVAILD ) {
            part_info->totoal_sector = 0 ; 
            part_info->start_sector = 0 ; 
            part_info->disk = (disk_t*)0 ; 
        } else {  // 设置这个表项的信息
            kernel_sprintf(part_info->name , "%s%d" , disk->name , i + 1 ) ; 

            part_info->disk = disk ; 
            part_info->start_sector = item->relative_sectors  ; 
            part_info->totoal_sector = item->total_sectors ;
        }

    }
}


static int identify(disk_t* disk ) {
    disk_send_cmd(disk , 0 , 0 , DISK_CMD_IDENTIFY ) ; 
    int err = inb(DISK_STATUS(disk) ) ;
    if(err == 0 ) {
        log_printf("%s disk not exists...." , disk->name) ; 
    }

    err = disk_wait_data(disk) ; 
    if(err < 0 ) {
        log_printf("disk[%s]: read failed.." , disk->name ) ; 
        return err ; 
    }

    uint16_t buf[256] ; 
    disk_read_data(disk , buf , sizeof(buf) ) ; 

    disk->sector_count = *((uint32_t*)(buf + 100) ) ; 
    disk->sector_size = SECTOR_SIZE ;  

    // 解析磁盘分区表
    partinfo_t* part = disk->partinfo + 0 ; 
    
    kernel_sprintf(part->name , "%s%d" , disk->name , 0 ) ; 
    part->disk = disk ; 
    part->start_sector = 0 ; 
    part->totoal_sector = disk->sector_count ; 
    part->type = FS_INVAILD ; 

    detect_part_info(disk) ; 

    return 0 ; 
}

static void print_disk_info(disk_t* disk ) {
    log_printf("%s" , disk->name) ; 
    log_printf("  port base: %x" , disk->port_base ) ; 
    log_printf("  total size: %d m" , disk->sector_count * disk->sector_size / 1024 / 1024) ; 

    for(int i = 0 ; i < DISK_PRIMARY_PART_CNT ; i ++ ) {
        partinfo_t * part_info = disk->partinfo + i ; 
        if(part_info->type != FS_INVAILD ) {
            log_printf("    %s: type:%x  start sector: %d  count:%d" , part_info->name , part_info->type 
            , part_info->start_sector , part_info->totoal_sector
            ) ; 
        }
    }
    
}

// 识别计算机中硬盘的个数
void disk_init() {
    // 这里仅考虑Primary bus 上的两个硬盘

    log_printf("Check disk.....") ;
    kernel_memset(disk_buf , 0 , sizeof(disk_buf) ) ; 
    
    for(int i = 0 ; i < DISK_PER_CHANNEL ; i ++ ) {
        disk_t* disk = disk_buf + i ; 
        
        // 设置磁盘名称
        kernel_sprintf(disk->name , "sd%c" , i + 'a' ) ; 
        
        disk->drive = (i == 0 ) ? DISK_MASTER : DISK_SLAVE ; 
        disk->port_base = IOBASE_PRIMARY ; 

        int err = identify(disk) ;   // 识别磁盘信息
        if(err == 0 ) { 
            print_disk_info(disk) ;  // 打印获取到的磁盘的信息
        }
    }

    // 解析磁盘分区表

}