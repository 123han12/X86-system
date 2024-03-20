#ifndef LIB_SYSCALL_H 
#define LIB_SYSCALL_H 
#include "os_cfg.h"
#include "core/syscall.h"
 
typedef struct  _syscall_args_t {
    int id ; // 系统函数调用号
    int arg0 ; 
    int arg1 ; 
    int arg2 ;
    int arg3 ; 

} syscall_args_t ; 

 

static inline int sys_call(syscall_args_t* args){
    
    uint32_t addr[] = { 0  , SELECTOR_SYSCALL | 0 } ; 
    int ret ; 
    __asm__ __volatile__ (
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"

        "lcalll *(%[a])\n\t"
        : "=a"(ret) 
        :[a]"r"(addr) , [id]"r"(args->id) , [arg0]"r"(args->arg0) , 
            [arg1]"r"(args->arg1) , [arg2]"r"(args->arg2) , [arg3]"r"(args->arg3) 
        : 
    ) ; 

    return ret ; 
} 


static inline void msleep(int ms ){
    if(ms <= 0 ) return ; 

    syscall_args_t args ; 
    args.id = SYS_sleep ;
    args.arg0 = ms ; 

    sys_call(&args) ;  
}

static inline int getpid(){

    syscall_args_t args ; 
    args.id = SYS_getpid ; 

    
    return sys_call(&args) ; 
   
}

//这个函数临时使用，函数原型较为特殊
static inline void print_msg(const char* fmt , int arg){
    syscall_args_t args ; 
    args.id = SYS_printmsg ; 
    args.arg0 = (int)fmt ; 
    args.arg1 = arg ; 

    sys_call(&args) ; 

}


#endif 