#ifndef FILE_H 
#define FILE_H 

#include "common/types.h"

#define FILE_NAME_SIZE  32 
#define FILE_TABLE_SIZE  2048

typedef enum _file_type_t {
    FILE_UNKNOWN = 0 , 
    FILE_TTY , 
} file_type_t ; 

// 用来描述一个文件
typedef struct _file_t {
    char file_name[FILE_NAME_SIZE] ;  // 文件的名称
    file_type_t type ; // 文件类型 
    uint32_t size ;  // 文件大小
    int ref ; // 文件打开的次数

    int dev_id ;  // 文件所属的dev_id
    int pos ;  // 文件读取的位置

    int mode ; // 文件的读写模式

} file_t ; 


void file_table_init(void) ; 
file_t* file_alloc(void) ; 
void file_free(file_t* file)  ;

void file_inc_ref(file_t* file) ; 

#endif 