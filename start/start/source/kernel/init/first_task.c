#include "tools/log.h"
#include "core/task.h"
#include "applib/lib_syscall.h"
#include "dev/tty.h"

int first_task_main(void)
{
#if 0 
    int id = getpid() ; 
    int count = 3;
    int pid = fork();
    if (pid < 0) {
        print_msg("create child proc failed. errorcode:%d\n" , pid ) ; 
    } else if (pid == 0) {
        // 子进程将执行的地方
        char * argv[] = {"han" , "shen" , "ao" } ;
        execve("/shell.elf", argv, (char **)0) ; 

    } else {
        print_msg("child task id=%d\n", pid);
        print_msg("parent: %d\n", count);
    }
    
#endif 

    for(int i = 0 ;i < 1 ; i ++ ) {
        int pid = fork() ;
        if(pid < 0 ) {
            print_msg("create shello failed.." , 0 ) ; 
            break ; 
        }
        if(pid == 0 ) {
            char tty_num[] = "/dev/tty?" ; 
            tty_num[sizeof(tty_num) - 2 ] = i + '0' ; 
            char* argv[] = {tty_num , (char*)0 } ; 
            execve("/shell.elf" , argv , (char**)0 ) ; 
            while(1) {
                msleep(1000) ;
            } 
        }
    }
    // 回收所有的孤儿进程的资源
    for(; ; )
    {
        int status ; 
        wait(&status) ; 
    }

    return 0;
}