#include "fs/devfs/devfs.h"
#include "dev/dev.h"
#include "fs/file.h"
#include "tools/klib.h"
#include "tools/log.h"


// 定义一个表，来表示设备文件系统支持的所有设备类型

static devfs_type_t dev_type_list[] = {
    {
        .name = "tty" , 
        .dev_type = DEV_TTY , 
        .file_type = FILE_TTY , 
    } , 
} ; 


int devfs_mount(struct _fs_t *fs, int major, int minor){
    fs->type = FS_DEVFS ; 
    return 0 ; 
}
int devfs_unmount(struct _fs_t *fs) {

}

int devfs_open(struct _fs_t *fs, const char *path, file_t *file){
    // path相当于是 tty0 tty1 
    for(int i = 0 ; i < sizeof(dev_type_list) / sizeof(dev_type_list[0])  ; i ++ ) {
        devfs_type_t* type = dev_type_list + i ; 
        int type_name_len = kernel_strlen(type->name ) ; 
        if(kernel_memcmp((void*)path , (void*)type->name , type_name_len) == 0 ) {
            int minor ; 
            if((kernel_strlen(path) > type_name_len ) && (path_to_num(path+type_name_len , &minor ) < 0 ) ) {
                log_printf("get device num failed: %s" , path ) ; 
                break ; 
            }

            int dev_id = dev_open(type->dev_type , minor , (void*)0 ) ; 
            if(dev_id < 0 ) {
                log_printf("open device failed:%s" , path ) ; 
                break ; 
            }
            file->dev_id = dev_id ; 
            file->fs = fs ; 
            file->pos = 0 ; 
            file->size = 0 ; 
            file->type = type->file_type ; 
            return 0 ; 
        } 
    }

    return -1 ;
}

int devfs_read(char *buf, int size, file_t *file){
    return dev_read(file->dev_id , file->pos , buf , size ) ; 
}

int devfs_write(char *buf, int size, file_t *file){
    return dev_write(file->dev_id , file->pos , buf , size ) ; 
}

void devfs_close(file_t *file) {
    dev_close(file->dev_id) ; 
}

int devfs_seek(file_t *file, uint32_t offset, int dir){
    return -1 ; 
}
int devfs_stat(file_t *file, struct stat *st){
    return -1 ; 
}

fs_op_t devfs_op = {
    .mount = devfs_mount  , 
    .unmount = devfs_unmount , 
    .open = devfs_open ,  
    .read = devfs_read , 
    .write = devfs_write ,  
    .close = devfs_close  , 
    .seek = devfs_seek , 
    .stat = devfs_stat , 

} ; 