
#include "lib_syscall.h"

int main (int argc, char **argv) {
    for(int i = 0 ; i < argc  ;  i ++ ) {
        print_msg("arg: %s" , (int)argv[i] ) ; 
    }

    fork() ; 
    // 子进程直接将运行权交出去，父进程继续运行
    yield() ;  
    for (;;) { 
        print_msg("shell pid=%d" , getpid() ) ; 
        msleep(1000);
    }
}