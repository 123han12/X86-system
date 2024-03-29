#ifndef LIB_SYSCALL_H 
#define LIB_SYSCALL_H 
#include "os_cfg.h"
#include "core/syscall.h"
#include <sys/stat.h>
 
typedef struct  _syscall_args_t {
    int id ; // 系统函数调用号
    int arg0 ; 
    int arg1 ; 
    int arg2 ;
    int arg3 ; 

} syscall_args_t ; 

void msleep(int ms ) ; 
int getpid() ; 
//这个函数临时使用，函数原型较为特殊
void print_msg(const char* fmt , int arg) ; 
int fork() ; 
// execve 加载应用程序到进程并切换到这个应用进程
int execve(const char* name , char* const * argv , char* const* env ) ; 
int yield(void) ; 

// 提供与文件系统相关的系统调用接口
int open(const char* name , int flags , ...) ; 
int read(int file , char* ptr , int len ) ; 
int write(int file , char* ptr , int len ) ; 
int close(int file) ; 
int lseek(int file , int ptr , int dir) ;  

int isatty(int file) ; 
int fstat(int file , struct stat* st);
void* sbrk(ptrdiff_t incr) ;  
int dup(int file) ; 

void _exit(int status) ;  
int  wait(int* status ) ; 

#endif 