#include "fs/fatfs/fatfs.h"
#include "dev/dev.h"
#include "tools/log.h"
#include "core/memory.h"
#include "tools/klib.h"
#include "common/types.h"
#include "fs/fs.h"
#include <sys/fcntl.h>



int fatfs_mount(struct _fs_t *fs, int major, int minor){
    int dev_id = dev_open(major , minor , (void*)0 ) ;
    if(dev_id < 0 ) {
        log_printf("open disk failed... major: %x , minor: %x" , major , minor ) ; 
        return -1 ; 
    } 

    dbr_t *dbr = (dbr_t*)memory_alloc_page() ; 
    if(!dbr) {
        log_printf("mount failed... can't alloc buf" ) ; 
        goto mount_failed ; 
    }

    int cnt = dev_read(dev_id , 0 , (char*)dbr , 1 ) ; 
    if(cnt < 1 ) {
        log_printf("read dbr failed....") ; 
        goto mount_failed ; 
    }

    fat_t* fat = &fs->fat_data ; 
    // 设置其属性

    fat->fat_buffer = (uint8_t*)dbr ; 
    
    fat->bytes_per_sec = dbr->BPB_BytsPerSec ; 
    fat->tbl_start = dbr->BPB_RsvdSecCnt ; 
    fat->tbl_sectors = dbr->BPB_FATSz16 ; 
    fat->tbl_cnt = dbr->BPB_NumFATs ; 
    fat->root_ent_cnt = dbr->BPB_RootEntCnt ; 
    fat->sec_per_cluster = dbr->BPB_SecPerClus ; 
    fat->root_start = fat->tbl_start + fat->tbl_sectors * fat->tbl_cnt ; 
    fat->data_start = fat->root_start + fat->root_ent_cnt * 32 / SECTOR_SIZE ; 
    fat->fs = fs ; 
    fat->cluster_byte_size = fat->bytes_per_sec * dbr->BPB_SecPerClus ; 
    fat->curr_sector = -1 ; 
    mutex_init(&fat->mutex) ; 
    fs->mutex = &fat->mutex ; 

    // 安全性检查
    if(fat->tbl_cnt != 2 ) {
        log_printf("fat table error: major: %x, minor: %x" , major , minor  ) ;
        goto mount_failed ; 
    }
    if(kernel_memcmp(dbr->BS_FileSysType  , "FAT16" , 5 ) != 0 ) {
        log_printf("not is a fat16 , major: %x , minor : %x" , major , minor ) ; 
        goto mount_failed ;  
    }

    fs->type = FS_FAT16 ; 
    fs->data = &fs->fat_data ; 
    fs->dev_id = dev_id ; 

    return 0; 
mount_failed:
    if(dbr ) {
        memory_free_page((uint32_t)dbr) ; 
    }
    dev_close(dev_id) ; 

    return -1 ; 
}
int fatfs_unmount(struct _fs_t *fs){

    fat_t* fat = (fat_t*)fs->data ;

    dev_close(fs->dev_id ) ; 
    memory_free_page((uint32_t)fat->fat_buffer) ; // 释放指定的缓冲区 

    return -1 ; 

}

void to_sfn(char* dest , const char* src ) {
    kernel_memset(dest , ' ' , SFN_LEN ); 
    char* curr = dest ; 
    char* end = dest + SFN_LEN ;
    while(*src && curr < end ) {
        char c = *src ++ ; 
        switch(c) {
            case '.':
                curr = dest + 8 ; 
                break ; 
            default:
                if((c >= 'a') && (c <= 'z' ) ) {
                    c = c - 'a' + 'A' ; 
                }
                *curr++ = c ; 
                break ; 
        }
    } 


}

int diritem_name_match(diritem_t* item , const char* path ) {
    char buf[SFN_LEN] ; 
    to_sfn(buf , path ) ; 
    return kernel_memcmp(buf , item->DIR_Name , SFN_LEN) == 0 ; 
}

file_type_t diritem_get_type(diritem_t* item ) {
    if(item->DIR_Attr & (DIRITEM_ATTR_VOLUME_ID | DIRITEM_ATTR_HIDDEN | DIRITEM_ATTR_SYSTEM ) ) {
        return FILE_UNKNOWN ; 
    }

    return item->DIR_Attr & DIRITEM_ATTR_DIRECTORY ? FILE_DIR : FILE_NORMAL ; 
}

static int bread_sector(fat_t* fat , int sector ) {
    
    // 表示当前缓存的扇区和需要的扇区是同一个扇区，直接返回
    if(fat->curr_sector == sector ) {
        return 0 ; 
    }

    int cnt = dev_read(fat->fs->dev_id , sector , fat->fat_buffer , 1) ; 
    if(cnt == 1 ) {
        fat->curr_sector = sector ; 
        return 0 ; 
    }

    return -1 ; 
}


void read_from_diritem(fat_t* fat , file_t* file , diritem_t* item , int index ) {
    file->type = diritem_get_type(item) ; 
    file->size = (int)item->DIR_FileSize ; 
    file->pos = 0 ; 
    file->sblk = (item->DIR_FstClusHI << 16) | item->DIR_FstClusL0 ; 
    file->cblk = file->sblk ; 
    file->p_index = index ; 


}

diritem_t* read_dir_entry(fat_t* fat , int index ) {
    if((index < 0 ) || (index >= fat->root_ent_cnt ) ) {
        return (diritem_t*)0 ; 
    }
    int offset = index * sizeof(diritem_t) ; 

    int err = bread_sector(fat , fat->root_start + offset / fat->bytes_per_sec ) ; 
    if(err < 0 ) {
        return (diritem_t*)0 ; 
    }

    return (diritem_t* ) (fat->fat_buffer + offset % fat->bytes_per_sec ) ; 

}

///缺省初始化diritem 
void diritem_init(diritem_t* item , uint8_t attr , const char* name ) {
    to_sfn((char*)item->DIR_Name , name ); 
    item->DIR_Attr = attr ; 
    item->DIR_FstClusHI = (uint16_t)(FAT_CLUSTER_INVALID >> 16) ; 
    item->DIR_FstClusL0 = (uint16_t)(FAT_CLUSTER_INVALID & 0xFFFF) ; 
    item->DIR_FileSize = 0 ; 
    item->DIR_NTRes = 0 ; 

    item->DIR_CrtTime = 0 ; 
    item->DIR_CrtDate = 0 ; 
    item->DIR_WrtTime = item->DIR_CrtTime ; 
    item->DIR_WrtDate = item->DIR_CrtDate ; 
    item->DIR_LastAccDate = item->DIR_CrtDate ; 
    return ; 
}

/// @brief 将 fat缓存中的内容写回到sector扇区中
/// @param fat 
/// @param sector 
/// @return 
int bwrite_sector(fat_t* fat , int sector ) {
    int cnt = dev_write(fat->fs->dev_id , sector , fat->fat_buffer , 1 ) ; 
    return (cnt == 1 ) ? 0 : -1 ; 
}


/// @brief 将本地的diritem表项写回到根目录区中的第index个表项中
/// @param fat 
/// @param item 
/// @param index 
/// @return 
int  write_dir_entry(fat_t* fat , diritem_t* item , int index) {
    if((index < 0 ) || (index >= fat->root_ent_cnt ) ) {
        return -1 ; 
    }
    int offset = index * sizeof(diritem_t) ; 
    int sector = fat->root_start + offset / fat->bytes_per_sec ; 
    int err = bread_sector(fat , sector) ; 
    if(err < 0 ) {
        return -1 ; 
    }
    // 找到相应的位置写入进去
    kernel_memcpy(fat->fat_buffer + offset % fat->bytes_per_sec , item , sizeof(diritem_t) ) ; 
    return bwrite_sector(fat , sector ) ; 
}

// 判断当前簇号是否合法
int cluster_is_vaild(cluster_t cluster ) {
    return (cluster < 0xFFF8) && (cluster >= 0x2);     // 值是否正确
}

cluster_t cluster_get_next(fat_t* fat , cluster_t curr ) {
    if(!cluster_is_vaild(curr) ) {
        return FAT_CLUSTER_INVALID ; 
    }
    int offset = curr * sizeof(cluster_t) ; 
    int sector = offset / fat->bytes_per_sec ; 
    int off_sector = offset % fat->bytes_per_sec ; 
    if(sector >= fat->tbl_sectors ) {
        log_printf("cluster too big. %d" , curr ) ;
        return FAT_CLUSTER_INVALID ; 
    }
    int err = bread_sector(fat , fat->tbl_start + sector ) ; 
    if(err < 0 ) {
        return FAT_CLUSTER_INVALID ; 
    }

    return *(cluster_t*)(fat->fat_buffer + off_sector ) ; 
}

// 将当前簇curr的下一个簇改为next
int cluster_set_next(fat_t* fat , cluster_t curr , cluster_t next ) {
    if(!cluster_is_vaild(curr) ) {
        return -1 ; 
    }
    int offset = curr * sizeof(cluster_t) ; 
    int sector = offset / fat->bytes_per_sec ; // 相对于FAT表的起始的扇区号
    int off_sector = offset % fat->bytes_per_sec ; 
    if(sector >= fat->tbl_sectors ) {
        log_printf("cluster too big. %d" , curr ) ; 
        return -1 ; 
    }

    int err = bread_sector(fat , fat->tbl_start + sector ); 
    if(err < 0 ) {
        return -1 ; 
    } 
    *(cluster_t*)(fat->fat_buffer + off_sector) = next ; 
    
    for(int i = 0 ; i < fat->tbl_cnt ; i ++ ) {
        err = bwrite_sector(fat , fat->tbl_start + sector ) ; 
        if(err < 0 ) {
            log_printf("write cluster failed.") ; 
            return -1 ; 
        }
        sector += fat->tbl_sectors ; 
    }
    return 0 ; 
}



/// @brief 释放以cluster开头的簇链
/// @param fat 
/// @param cluster 
/// @return 
int cluster_free_chain(fat_t* fat , int start ) {
    while(cluster_is_vaild(start) ) {
        cluster_t next = cluster_get_next(fat , start ) ; 
        cluster_set_next(fat , start , FAT_CLUSTER_INVALID ) ; 
        start = next ; 
    }
    return 0 ; 
}

// 例如 path 相当于file.c
int fatfs_open(struct _fs_t *fs, const char *path, file_t *file){
    fat_t* fat = (fat_t*)fs->data ;
    diritem_t* file_item = (diritem_t*)0 ; 
    int p_index = -1 ;  // 表示file 在根目录区中的下标

    for(int i = 0 ; i < fat->root_ent_cnt ; i ++ ) {
        diritem_t* item = read_dir_entry(fat , i ) ; 
        if(item == (diritem_t*)0 ) {
            return -1 ; 
        }
        if(item->DIR_Name[0] == DIRITEM_NAME_END) {
            p_index = i ; 
            break ; 
        }   
        if(item->DIR_Name[0] == DIRITEM_NAME_FREE) {
            p_index = i ; 
            continue ; 
        }

        if(diritem_name_match(item , path )) {
            file_item = item ; 
            p_index = i ; 
            break ; 
        }
    }

    if(file_item ) {
        read_from_diritem(fat , file , file_item , p_index ) ; 

        if(file->mode & O_TRUNC ) {
            cluster_free_chain(fat , file->sblk ) ; 
            file->cblk = file->sblk = FAT_CLUSTER_INVALID ; 
            file->size = 0 ; 
        }
        return 0 ; 
    } else if((file->mode & O_CREAT ) && ( p_index != -1 ) ) {
        diritem_t item ; 
        diritem_init(&item , 0 , path) ; 
        int err = write_dir_entry(fat , &item , p_index) ; 
        if(err < 0 ) {
            log_printf("create file failed.") ; 
            return -1 ; 
        }
        read_from_diritem(fat , file , &item , p_index ) ; 
        return 0 ; 
    } else {
        return -1 ; 
    }
    return 0 ; 
}

/// @brief 分配 一个大小为cluster_cnt的簇链
/// @param fat 
/// @param cluster_cnt 
/// @return 
cluster_t cluster_alloc_free(fat_t* fat , int cluster_cnt ) {
    cluster_t pre , curr , start ; 
    int c_total = fat->tbl_sectors * fat->bytes_per_sec / sizeof(cluster_t) ; 

    pre = start = FAT_CLUSTER_INVALID ; 
    for(curr = 2 ; (curr < c_total) && cluster_cnt ; curr ++ ) {
        cluster_t free = cluster_get_next(fat , curr) ; 
        if(free == FAT_CLUSTER_FREE ) {
            if(!cluster_is_vaild(start) ) {
                start = curr ; 
            }

            if(cluster_is_vaild(pre) ) {
                int err = cluster_set_next(fat , pre , curr ); 
                if(err < 0 ) {
                    cluster_free_chain(fat , start ) ; 
                    return FAT_CLUSTER_INVALID ; 
                }
            }
            
            pre = curr ; 
            cluster_cnt -- ; 
        }
    }

    if(cluster_cnt == 0 ) {
        int err = cluster_set_next(fat , pre , FAT_CLUSTER_INVALID ) ; 
        if(err == 0 ) {
            return start ; 
        }
    }
    cluster_free_chain(fat , start ) ; 
    return FAT_CLUSTER_INVALID ; 
}

/// @brief 将file文件扩展inc_size个bytes
/// @param file 
/// @param inc_size 
/// @return 
int expand_file(file_t* file , int inc_bytes ) {
    fat_t* fat = (fat_t*)file->fs->data ; 
    
    int cluster_cnt ; 
    if((file->size == 0 ) || (file->size % fat->cluster_byte_size == 0 ) ) {
        cluster_cnt = up2(inc_bytes , fat->cluster_byte_size ) / fat->cluster_byte_size ; 
    }else {
        int cfree = fat->cluster_byte_size - (file->size % fat->cluster_byte_size ) ; 
        if(cfree > inc_bytes ) {
            return 0 ; 
        }
        cluster_cnt = up2(inc_bytes - cfree , fat->cluster_byte_size ) / fat->cluster_byte_size ; 
        if(cluster_cnt == 0 ) {
            cluster_cnt = 1 ; 
        }
    }

    cluster_t start = cluster_alloc_free(fat , cluster_cnt) ; 
    if(!cluster_is_vaild(start) ) {
        log_printf("no cluster for file write") ; 
        return -1 ; 
    }

    if(!cluster_is_vaild(file->sblk) ) {
        file->cblk = file->sblk = start ; 
    }else {
        int err = cluster_set_next(fat , file->cblk , start ) ; 
        if(err < 0 ) {
            return -1 ; 
        }
    }

    return 0 ; 
}



int move_file_pos(file_t* file , fat_t* fat , int move_bytes , int expand ) {
    uint32_t c_offset = file->pos % fat->cluster_byte_size ;  // 当前位置在当前簇的偏移
    
    // 如果跨簇，这里需要修改簇号。也就是 file->cblk
    if(c_offset + move_bytes >= fat->cluster_byte_size ) {
        cluster_t next = cluster_get_next(fat , file->cblk ) ; 
        if(next == FAT_CLUSTER_INVALID && expand ) {
            //, 这里实现文件的自扩展。
            int err = expand_file(file , fat->cluster_byte_size ) ; 
            if(err < 0 ){
                return -1 ; 
            }
            next = cluster_get_next(fat , file->cblk) ; 
        }
        file->cblk = next  ; 
    }
    file->pos += move_bytes ; 

    return 0 ; 
}

int fatfs_read(char *buf, int size, file_t *file){
    fat_t* fat = (fat_t*)file->fs->data ;
    uint32_t nbytes = size ; 
    if(nbytes + file->pos >= file->size) {
        nbytes = file->size - file->pos ; 
    }
    uint32_t total_read = 0 ; 
    while(nbytes > 0 ) {
        uint32_t curr_read = nbytes ; 
        uint32_t cluster_offset = file->pos % fat->cluster_byte_size ; 
        uint32_t start_sector = fat->data_start + (file->cblk - 2)* fat->sec_per_cluster ; 

        if((cluster_offset == 0 ) && (curr_read >= fat->cluster_byte_size ) ) {
            int err = dev_read(fat->fs->dev_id , start_sector , buf , fat->sec_per_cluster ) ; 
            if(err < 0 ) {
                return total_read ; 
            }
            curr_read = fat->cluster_byte_size ; 
        } else {
            if(cluster_offset + curr_read > fat->cluster_byte_size ) {
                curr_read = fat->cluster_byte_size - cluster_offset ; 
            }

            fat->curr_sector = -1 ; 
            int err = dev_read(fat->fs->dev_id , start_sector , fat->fat_buffer ,  fat->sec_per_cluster ) ; 
            if(err < 0 ) {
                return total_read ; 
            }
            kernel_memcpy(buf , fat->fat_buffer + cluster_offset , curr_read) ; 
        }

        buf += curr_read ; 
        nbytes -= curr_read ; 
        total_read += curr_read ; 
        int err = move_file_pos(file , fat , curr_read , 0 ) ;  // cblk 一次最多移动一个
        if(err < 0 ) {
            return total_read ; 
        }
    }

    return total_read ; 
}






/// @brief 返回值是写入的字节的大小
/// @param buf 
/// @param size 
/// @param file 
/// @return 
int fatfs_write(char *buf, int size, file_t *file){
    fat_t* fat = (fat_t*)file->fs->data ;
    if(file->pos + size > file->size ) {
        int inc_size = file->pos + size - file->size ; 
        int err = expand_file(file , inc_size) ; 
        if(err < 0 ) {
            return 0 ;   
        }
    }

    uint32_t nbytes = size ; 
    uint32_t total_write = 0 ; 
    while(nbytes) {
        uint32_t curr_write = nbytes ; 
        uint32_t cluster_offset = file->pos % fat->cluster_byte_size ;  
        uint32_t start_sector = fat->data_start + (file->cblk - 2 )* fat->sec_per_cluster ; 
        // 数据区的簇号是从0开始的
        if((cluster_offset == 0 ) && (nbytes >= fat->cluster_byte_size ) ) {
            int err = dev_write(fat->fs->dev_id , start_sector , buf , fat->sec_per_cluster ) ; 
            if(err < 0 ) {
                return total_write ; 
            }
            curr_write = fat->cluster_byte_size ; // 当前写入的数据就是一个簇的大小
        }else {
            if(cluster_offset + curr_write > fat->cluster_byte_size ) {
                curr_write = fat->cluster_byte_size - cluster_offset ; 
            }
            fat->curr_sector = -1 ; 
            int err = dev_read(fat->fs->dev_id , start_sector , fat->fat_buffer , fat->sec_per_cluster ) ; 
            if(err < 0 ) {
                return  total_write ;  
            }
            kernel_memcpy(fat->fat_buffer + cluster_offset , buf , curr_write ) ; 
            
            err = dev_write(fat->fs->dev_id , start_sector , fat->fat_buffer  , fat->sec_per_cluster ) ; 
            if(err < 0 ) {
                return total_write ; 
            }
        }


        nbytes -= curr_write ; 
        total_write += curr_write ; 
        buf += curr_write ; 
        file->size += curr_write ; 
        int err = move_file_pos(file , fat , curr_write , 1 ) ;  // 自扩充
        if(err < 0 ) {
            return total_write ; 
        }
    }

    return total_write ; 

}
void fatfs_close(file_t *file){
    
    // 回写file中的数据
    if(file->mode = O_RDONLY ) {
        return ; 
    }

    fat_t* fat = (fat_t*)file->fs->data ; 
    diritem_t* item = read_dir_entry(fat , file->p_index ) ; 
    if(item == (diritem_t*)0 ) {
        return ; 
    }

    item->DIR_FileSize = file->size ; 
    item->DIR_FstClusHI = (uint16_t)(file->sblk >> 16 ) ; 
    item->DIR_FstClusL0 = (uint16_t)(file->sblk & 0xFFFF ) ; 
    write_dir_entry(fat , item , file->p_index ) ; 

    return ; 
}

/// @brief 将pos调整到相对于文件开头的offset的地方
/// @param file 
/// @param offset 
/// @param dir 
/// @return 
int fatfs_seek(file_t *file, uint32_t offset, int dir){
    // 如果不是相对于文件开头，就不处理了
    if(dir != 0 ) {
        return -1 ; 
    } 
    fat_t* fat =  (fat_t*)file->fs->data ; 
    cluster_t current_cluster = file->sblk ; 
    uint32_t curr_pos = 0 ; 
    uint32_t offset_to_move = offset ; 
    while(offset_to_move > 0 ) {
        uint32_t c_offset = curr_pos % fat->cluster_byte_size ; 
        uint32_t curr_move = offset_to_move ; 
        if(c_offset + curr_move <  fat->cluster_byte_size ) {
            curr_pos += curr_move ; 
            break ; 
        }   

        curr_move = fat->cluster_byte_size - c_offset ; 
        curr_pos += curr_move ; 
        offset_to_move -= curr_move ; 
        current_cluster  = cluster_get_next(fat , current_cluster ) ; 
        if(!cluster_is_vaild(current_cluster) ) {
            return -1 ; 
        }
    }

    file->pos = curr_pos ; 
    file->cblk = current_cluster ; 
    return 0 ; 
}
int fatfs_stat(file_t *file, struct stat *st){

    return -1 ;  
}







static void diritem_get_name(diritem_t* item , char* dest ) {
    char* c = dest ; 
    char* ext = (char*)0 ; 
    kernel_memset(dest , 0 , 12 ) ; 
    for(int i = 0 ; i < 11 ; i ++ ) {
        if(item->DIR_Name[i] != ' ' ) {
            *c++ = item->DIR_Name[i] ; 
        }
        if(i == 7 ) {
            ext = c ; 
            *c++ = '.' ; 
        }
    } 
    if(ext && ext[1] == '\0' ) {
        ext[0] = '\0' ; 
    }
    return ; 
}

int fatfs_opendir (struct _fs_t* fs , const char* name , DIR* dir ){
    dir->index = 0 ; 
    return 0 ; 
} 
int fatfs_readdir(struct _fs_t* fs , DIR* dir , dirent* dirent ) {
    fat_t* fat = (fat_t*)fs->data ; 
    while(dir->index < fat->root_ent_cnt ) {
        diritem_t * item = read_dir_entry(fat , dir->index) ; 
        if(item == (diritem_t*)0 ) {
            return -1 ; 
        }
        
        if(item->DIR_Name[0] == DIRITEM_NAME_END ) {
            break ; 
        } 

        if(item->DIR_Name[0] != DIRITEM_NAME_FREE ) {
            file_type_t type = diritem_get_type(item ) ; 
            if((type == FILE_NORMAL ) || (type == FILE_DIR) ) {
                dirent->index = dir->index ++ ; 
                dirent->type = type ; 
                dirent->size = item->DIR_FileSize ; 
                // 将item 中的名称的信息放入到dirent->name 中
                diritem_get_name(item , dirent->name ) ; 
                return 0 ; 
            }
        } 
        dir->index ++ ; 
    }

    return -1 ; 
}
int fatfs_closedir (struct _fs_t* fs , DIR* dir ) {
    return 0 ; 
}







int fatfs_unlink (struct _fs_t* fs , const char* filepath){
    // 1.释放簇链
    // 2.清空对应的目录项
    fat_t* fat = (fat_t*)fs->data ; 
    
    for(int i = 0 ; i < fat->root_ent_cnt ; i ++ ) {
        diritem_t* item = read_dir_entry(fat , i ) ; 
        if(item == (diritem_t*)0 ) {
            return -1 ; 
        }

        if(item->DIR_Name[0] == DIRITEM_NAME_END ) {
            break ; 
        }
        if(item->DIR_Name[0] == DIRITEM_NAME_FREE) {
            continue ; 
        }
        if(diritem_name_match(item , filepath) ) {
            // 释放当前文件对应的簇链
            int cluster = (item->DIR_FstClusHI << 16 ) | (item->DIR_FstClusL0 ) ; 
            cluster_free_chain(fat , cluster ) ; 

            diritem_t item ; 
            kernel_memset(&item , 0 , sizeof(diritem_t) ) ; 
            item.DIR_Name[0] = DIRITEM_NAME_FREE ; 
            return write_dir_entry(fat , &item , i ) ; 
        }
    }


    return -1 ;
}

fs_op_t fatfs_op = {
    .mount = fatfs_mount,
    .unmount = fatfs_unmount,
    .open = fatfs_open,
    .read = fatfs_read,
    .write = fatfs_write,
    .close = fatfs_close,
    .seek = fatfs_seek,
    .stat = fatfs_stat,
    .opendir = fatfs_opendir , 
    .readdir = fatfs_readdir , 
    .closedir = fatfs_closedir , 
    .unlink = fatfs_unlink , 
};
