#ifndef DISK_H 
#define DISK_H 
#include "common/types.h"

#define DISK_NAME_SIZE         32 
#define DISK_PRIMARY_PART_CNT  (4 + 1)
#define PART_NAME_SIZE         32 
#define DISK_CNT               2 
#define DISK_PER_CHANNEL       2 

#define IOBASE_PRIMARY         0x1F0 
#define DISK_DATA(disk)        (disk->port_base + 0)   
#define DISK_ERROR(disk)       (disk->port_base + 1 ) 
#define DISK_SECTOR_COUNT(disk)       (disk->port_base + 2 ) 
#define DISK_LBA_LO(disk)       (disk->port_base + 3 ) 
#define DISK_LBA_MID(disk)       (disk->port_base + 4 ) 
#define DISK_LBA_HI(disk)       (disk->port_base + 5 ) 
#define DISK_DRIVE(disk)       (disk->port_base + 6 ) 
#define DISK_STATUS(disk)       (disk->port_base + 7 ) 
#define DISK_CMD(disk)       (disk->port_base + 7 ) 


#define DISK_CMD_READ           0x24 
#define DISK_CMD_WRITE          0x34
#define DISK_CMD_IDENTIFY       0xEC  

#define DISK_STATUS_ERR          (1 << 0 ) 
#define DISK_STATUS_DRQ          (1 << 3 ) 
#define DISK_STATUS_DF           (1 << 5 ) 
#define DISK_STATUS_BUSY         (1 << 7 )

#define DISK_DRIVE_BASE          0xE0 

#define MBR_PRIMARY_PART_NR      4 


#pragma pack(1) 

struct _disk_t ; 

typedef struct _partinfo_t {
    char name[PART_NAME_SIZE] ; 
    struct _disk_t* disk ; // 当前分区的所属磁盘
    enum {
        FS_INVAILD = 0x00 , 

        FS_FAT16_0 = 0x6  , 
        FS_FAT16_1 = 0xE  , 
    } type ; 
    int start_sector ; 
    int totoal_sector ;  


} partinfo_t ; 

// 扩展分区的概念
typedef struct _disk_t {
    char name[DISK_NAME_SIZE] ; 
    
    enum {  
        DISK_MASTER = (0 << 4 ), 
        DISK_SLAVE = (1 << 4 ), 
    }drive ; 

    // 端口起始地址 , 一共八个端口
    uint16_t port_base ; 


    int sector_size ; // 当前磁盘的扇区的大小
    int sector_count ; // 扇区的数量
    partinfo_t partinfo[DISK_PRIMARY_PART_CNT] ; 


} disk_t ; 

// 用于分区表的结构解析的结构体



// 用于描述分区表中的表项的结构体
typedef struct _part_item_t {
    uint8_t boot_active ;  // 引导标志
    uint8_t start_header ; 
    uint16_t start_sector:6 ; 
    uint16_t start_cylinder:10 ; 
    uint8_t system_id ; 
    uint8_t end_header ; 
    uint16_t end_sector:6 ; 
    uint16_t end_cylinder: 10 ; 
    uint32_t  relative_sectors ;   // 起始扇区号
    uint32_t total_sectors ;   // 扇区数量

} part_item_t ; 


typedef struct _mbr_t {
    uint8_t code[446] ;
    part_item_t part_item[MBR_PRIMARY_PART_NR] ; 
    uint8_t boot_sig[2] ; 


}mbr_t ; 

#pragma pack() 




void disk_init() ; 





#endif 