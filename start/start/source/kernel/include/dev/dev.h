#ifndef DEV_H 
#define DEV_H 

#define DEV_NAME_SIZE      32

#define DEV_TTY            1 

enum {
    DEV_UNKNOWN = 0 , 
} ; 



struct _dev_desc_t ; 
// 用于表述某一种特定的设备
typedef struct _device_t {
    struct _dev_desc_t* desc ;  // 设备的特性
    
    int mode ; // 用来控制模式

    int minor ; // 次设备号

    void* data ; // 存放相关参数

    int open_count ; // 设备打开次数

}device_t ; 


// 描述某一类的特性
typedef struct _dev_desc_t {
    char name[DEV_NAME_SIZE] ; // 设备名称
    int major ;  // 主设备类型号


    int (*open)(device_t* dev ) ;  // open 函数，不同的函数逻辑不同，所以这里是函数指针
    int(*read)(device_t* dev , int addr , char* buf , int size ) ; 
    int(*write)(device_t* dev , int addr , char* buf , int size ) ; 
    int(*control)(device_t* dev , int cmd , int arg0 , int agr1 ) ; 
    int(*close)(device_t* dev) ; 

} dev_desc_t ; 

int dev_open(int major , int minor , void *data ) ; 
int dev_read(int dev_id , int addr , char* buf , int size ) ; 
int dev_write(int dev_id , int addr , char* buf , int size) ;
int dev_control(int dev_id , int cmd , int arg0 , int arg1 ) ; 
void dev_close(int dev_id) ; 


#endif 