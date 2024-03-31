#ifndef FS_H 
#define FS_H 
#include <sys/stat.h>
#include "file.h"
#include "tools/list.h"
#include "ipc/mutex.h"
#include "fs/fatfs/fatfs.h"
struct _fs_t ; 

//函数注册表
typedef struct _fs_op_t {
    int (*mount)(struct  _fs_t* fs , int major , int minor ) ; 
    int (*unmount)(struct _fs_t* fs ) ; 

    int (*open)(struct _fs_t* fs , const char* path , file_t* file ); 
    int (*read)(char* buf , int size , file_t* file ) ; 
    int (*write)(char* buf , int size , file_t* file ) ; 
    void (*close)(file_t* file ) ; 
    int (*seek)(file_t* file , uint32_t offset , int dir ) ; 
    int (*stat)(file_t* file , struct stat* st ) ; // 取出file文件的信息放入到st结构体中
} fs_op_t ; 


#define FS_MOUNT_SIZE   512 

typedef enum _fs_type_t {
    FS_DEVFS  , 
    FS_FAT16  , 

} fs_type_t ; 

typedef struct _fs_t {
    fs_op_t * op ; // 回调函数的表
    char mount_point[FS_MOUNT_SIZE] ; 
    fs_type_t type ; 
    void*  data ; 
    int dev_id ;   // 设备的id , 识别磁盘上的某个分区
    
    list_node_t node ; 
    mutex_t*  mutex ; 
    
    union {
        fat_t fat_data ; 
    } ; 
    
}fs_t ; 


int sys_open(const char* name , int flags ) ; 
int sys_read(int file , char* ptr , int len ) ; 
int sys_write(int file , char* ptr , int len) ;
int sys_lseek(int file , int ptr , int dir ) ;

int sys_close(int file) ; 

int sys_fstat(int file , struct stat* st ) ; 
int sys_isatty(int file) ; 

void fs_init(void) ; 

int sys_dup(int file ) ; 



int path_to_num(const char* path , int* num ) ; 
const char* path_next_child(const char* path ) ; 




#endif 