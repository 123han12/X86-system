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

 

static inline void sys_call(syscall_args_t* args){
    
    uint32_t addr[] = { 0  , SELECTOR_SYSCALL | 0 } ; 
    
    __asm__ __volatile__ (
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"

        "lcalll *(%[a])\n\t"
        : 
        :[a]"r"(addr) , [id]"r"(args->id) , [arg0]"r"(args->arg0) , 
            [arg1]"r"(args->arg1) , [arg2]"r"(args->arg2) , [arg3]"r"(args->arg3) 
        : 
    ) ; 
} 


static inline void msleep(int ms ){
    if(ms <= 0 ) return ; 

    syscall_args_t args ; 
    args.id = SYS_sleep ;
    args.arg0 = ms ; 

    sys_call(&args) ;  
}

#endif 