#include "lib_syscall.h" 
 
static inline int sys_call(syscall_args_t* args){
    
    uint32_t addr[] = { 0  , SELECTOR_SYSCALL | 0 } ; 
    int ret ; 
    __asm__ __volatile__ (   // 这里使用汇编进行手动压栈的话不用进行手动出栈的吗？
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


void msleep(int ms ){
    if(ms <= 0 ) return ; 

    syscall_args_t args ; 
    args.id = SYS_sleep ;
    args.arg0 = ms ; 

    sys_call(&args) ;  
}

int getpid(){

    syscall_args_t args ; 
    args.id = SYS_getpid ; 

    
    return sys_call(&args) ; 
   
}

//这个函数临时使用，函数原型较为特殊
void print_msg(const char* fmt , int arg){
    syscall_args_t args ; 
    args.id = SYS_printmsg ; 
    args.arg0 = (int)fmt ; 
    args.arg1 = arg ; 

    sys_call(&args) ; 
}

int fork(){
    syscall_args_t args ; 
    args.id = SYS_fork ; 

    
    return sys_call(&args) ; 

}

// execve 加载应用程序到进程并切换到这个应用进程
int execve(const char* name , char* const * argv , char* const* env ){
    syscall_args_t args ; 
    args.id = SYS_execve ; 
    args.arg0 = (int)name ; 
    args.arg1 = (int)argv ;
    args.arg2 = (int)env ; 
    return sys_call(&args) ;

}

int yield(void){
    
    syscall_args_t args ; 
    args.id = SYS_yield ; 
    
    return sys_call(&args) ; 
}

// 底层调用 sys_open 如果name 为 "tty:0" 则调用dev_open 
int open(const char* name , int flags , ...) {
    syscall_args_t args ; 
    args.id = SYS_open ; 
    args.arg0 = (int)name ; 
    args.arg1 = (int)flags ;

    return sys_call(&args) ;
} 
int read(int file , char* ptr , int len ) {
    syscall_args_t args ; 
    args.id = SYS_read ; 
    args.arg0 = (int)file ; 
    args.arg1 = (int)ptr ;
    args.arg2 = (int)len ; 

    return sys_call(&args) ;
} 
int write(int file , char* ptr , int len ) {
    syscall_args_t args ; 
    args.id = SYS_write ; 
    args.arg0 = (int)file ; 
    args.arg1 = (int)ptr ;
    args.arg2 = (int)len ; 

    return sys_call(&args) ;
}
int close(int file) {
    syscall_args_t args ; 
    args.id = SYS_close ; 
    args.arg0 = (int)file ; 
    return sys_call(&args) ;
}
int lseek(int file , int ptr , int dir) {
    syscall_args_t args ; 
    args.id = SYS_lseek ; 
    args.arg0 = (int)file ;
    args.arg1 = (int)ptr ; 
    args.arg2 = (int)dir ;  
    return sys_call(&args) ;
}


// 实现newlib库对底层的库函数的接口的要求。


int isatty(int file) {
    syscall_args_t args ; 
    args.id = SYS_isatty ; 
    args.arg0 = (int)file ; 
    return sys_call(&args) ;
}

int fstat(int file , struct stat* st){
    syscall_args_t args ; 
    args.id = SYS_fstat ; 
    args.arg0 = (int)file ; 
    args.arg1 = (int)st ; 
    return sys_call(&args) ;

}

void* sbrk(ptrdiff_t incr) {
    syscall_args_t args ; 
    args.id = SYS_sbrk; 
    args.arg0 = (int)incr; 
    return (void*)sys_call(&args) ;
} 

int dup(int file) {
    syscall_args_t args ; 
    args.id = SYS_dup ; 
    args.arg0 = (int)file ; 
    return sys_call(&args) ;
}



void _exit(int status) {
    syscall_args_t args ; 
    args.id = SYS_exit ; 
    args.arg0 = (int)status ; 
    sys_call(&args) ;
    for(; ; ) {
        
    }
}

int wait(int* status ) {
    syscall_args_t args ; 
    args.id = SYS_wait ; 
    args.arg0 = (int)status ; 
    return sys_call(&args) ;
}