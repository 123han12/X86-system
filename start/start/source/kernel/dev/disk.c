#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "common/cpu_instr.h"
#include "common/boot_info.h"

static mutex_t mutex ; 
static sem_t  op_sem ; 

static disk_t disk_buf[DISK_CNT] ; 

static int task_on_op ; 




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

/// @brief 识别 primary bus 上的硬盘，并且将识别到的信息放入到disk_t 结构中
/// @param disk 
/// @return 
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

// 识别计算机中硬盘的个数, 将通过端口读取到的disk1 和 disk2 的信息存储到 disk_buf[0] 和disk_buf[1] 中
void disk_init() {
    // 这里仅考虑Primary bus 上的两个硬盘

    log_printf("Check disk.....") ;
    kernel_memset(disk_buf , 0 , sizeof(disk_buf) ) ; 

    // 进行锁的初始化
    mutex_init(&mutex) ; 
    sem_init(&op_sem , 1 ) ; 

    
    for(int i = 0 ; i < DISK_PER_CHANNEL ; i ++ ) {
        disk_t* disk = disk_buf + i ; 

        
        // 设置磁盘名称
        kernel_sprintf(disk->name , "sd%c" , i + 'a' ) ; 
        
        disk->drive = (i == 0 ) ? DISK_MASTER : DISK_SLAVE ; 
        disk->port_base = IOBASE_PRIMARY ; 
        disk->mutex = &mutex ;
        disk->op_sem = &op_sem ; 

        int err = identify(disk) ;   // 识别磁盘信息
        if(err == 0 ) { 
            print_disk_info(disk) ;  // 打印获取到的磁盘的信息
        }
    }

    // 解析磁盘分区表

}



int disk_open(device_t* dev ) {
    // 0xa0 a表示磁盘的编号，0表示 分区号
    int disk_idx = ( ( dev->minor >> 4 ) & 0xF )  - 0xa; 
    int part_idx = dev->minor & 0xF ;  // 分区号

    if((disk_idx >= DISK_CNT ) || (part_idx >= DISK_PRIMARY_PART_CNT ) ) {
        log_printf("device minor error: %d" , dev->minor ) ;
        return -1 ;  
    } 

    disk_t* disk = disk_buf + disk_idx ; 
    if(disk->sector_count == 0 ) {
        log_printf("disk not exists,device: sd%x" , dev->minor ) ; 
        return -1 ; 
    }
    partinfo_t* part_info = disk->partinfo + part_idx ; 
    if(part_info->totoal_sector == 0 ) {
        log_printf("part not exists, device: sd%x" , dev->minor ) ;  
        return -1 ; 
    }

    // 将目标分区的信息存储到dev->data中
    dev->data = part_info ; 

    // 使得磁盘中断能够生效
    irq_install(IRQ14_HARDDISK_PRIMARY , (irq_handler_t)exception_handler_ide_primary) ; 
    irq_enable(IRQ14_HARDDISK_PRIMARY) ; 
    

    return 0 ; 
    
}

/// @brief addr 参数表示的是相对于当前分区的扇区的起始地址 , buf 表示要读到的地方，size表示读取多少个扇区。
/// @param dev 
/// @param addr 
/// @param buf 
/// @param size 
/// @return 
int disk_read(device_t* dev , int addr , char* buf , int size ) {

    // 通过信号量和中断的配合防止出现进程的忙等现象
    partinfo_t* part_info = (partinfo_t*)dev->data ;
    if(!part_info) {
        log_printf("Get part info failed. device: %d" , dev->minor ) ; 
    }

    disk_t* disk = part_info->disk ; 

    if(disk == (disk_t*)0 ) {
        log_printf("No disk:%d" , dev->minor);
    }

    mutex_lock(disk->mutex) ;  // 先进行上锁

    task_on_op = 1 ; 

    // 在传入的时候需要将相对分区地址转换为绝对分区地址
    disk_send_cmd(disk , part_info->start_sector + addr , size , DISK_CMD_READ ) ; 
    
    int cnt ; 
    for(cnt = 0 ; cnt < size ; cnt ++ , buf += disk->sector_size ) {
        if(task_current() ) {
            sem_wait(disk->op_sem ) ; 
        }
        int err = disk_wait_data(disk) ; 
        if(err < 0 ) {
            log_printf("disk(%s)  read error: start sector %d count: %d" , addr  , size ) ; 
            break ; 
        }
        // 每一次读取一个扇区的大小
        disk_read_data(disk , buf , disk->sector_size ) ; 
    }

    mutex_unlock(disk->mutex) ; 


    return cnt ; 
}

/// @brief addr 参数表示的是相对于当前分区的扇区的起始地址 , buf 表示要写入的数据的起始地址，size表示写入多少扇区
/// @param dev 
/// @param addr 
/// @param buf 
/// @param size 
/// @return 
int disk_write(device_t* dev , int addr , char* buf , int size ) {
    // 通过信号量和中断的配合防止出现进程的忙等现象
    partinfo_t* part_info = (partinfo_t*)dev->data ;
    if(!part_info) {
        log_printf("Get part info failed. device: %d" , dev->minor ) ; 
    }

    disk_t* disk = part_info->disk ; 

    if(disk == (disk_t*)0 ) {
        log_printf("No disk:%d" , dev->minor);
    }

    mutex_lock(disk->mutex) ;  // 先进行上锁

    task_on_op = 1 ; 

    // 在传入的时候需要将相对分区地址转换为绝对分区地址
    disk_send_cmd(disk , part_info->start_sector + addr , size , DISK_CMD_WRITE ) ; 
    
    int cnt ; 
    for(cnt = 0 ; cnt < size ; cnt ++ , buf += disk->sector_size ) {
        disk_write_data(disk , buf , disk->sector_size ) ;  

        if(task_current() ) {
            sem_wait(disk->op_sem ) ; 
        }
        int err = disk_wait_data(disk) ; 
        if(err < 0 ) {
            log_printf("disk(%s)  read error: start sector %d count: %d" , addr  , size ) ; 
            break ; 
        }
        // 每一次读取一个扇区的大小
    }

    mutex_unlock(disk->mutex) ; 

    return cnt ; 
}
int disk_control(device_t* dev , int cmd , int arg0 , int agr1 ) {
    return -1 ; 
}
int disk_close(device_t* dev) {
    return -1 ; 
}


dev_desc_t dev_disk_desc = {
    .name = "disk", 
    .major = DEV_DISK , 
    .open = disk_open , 
    .read = disk_read , 
    .write = disk_write , 
    .control = disk_control , 
    .close = disk_close , 
} ; 


void do_handler_ide_primary(exception_frame_t* frame ) {
    pic_send_eoi(IRQ14_HARDDISK_PRIMARY) ;  


    if(task_on_op && task_current() ) { 
        sem_notify(&op_sem) ; // 通知进程
    }

}



