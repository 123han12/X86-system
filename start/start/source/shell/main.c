
#include "lib_syscall.h"
#include <stdio.h>

int main (int argc, char **argv) {

    sbrk(0) ;   

    sbrk(100) ; 
    sbrk(200) ; 
    sbrk(4096 * 2 + 2 ) ;
    sbrk(4096 * 5 + 89 ) ; 
    
    printf("abef\b\b\b\bcd\n" ) ;  // \b 表示将光标左移一位
    printf("abcd\x7f;fg\n") ;  // 0x7f 表示向左删除一个字符，并且在c语言字符串中想要表示16进制数的话开头需要写\x

    /*
        \033表示 ESC字符， ESC 7 表示存储此时光标的位置， ESC 8 表示将光标的位置恢复到此处。
    */
    printf("\0337Hello,World!\0338han\n") ;   // 结果应该是 hanlo,World!


    printf("\033[31;42mHello World!\033[39;49m\n") ; // Hello World! 字符的前景色为红色背景色为绿色 


    printf("123\033[2DHello,world!\n") ;  // 光标左移2， 应该输出的是1Hello,world!
    printf("123\033[2CHello,world!\n") ; // 光标右移2   结果为 123  Hello,world!

    printf("\033[31m\n") ; // ESC [pn m , Hello,world 红色，其余为绿色
    printf("\033[10;10H test!\n") ; // 定位到10 , 10 , test! 
    printf("\033[20;20H test!\n") ; // 定位到20 , 20 , test! 
    printf("\n") ; 
    printf("\033[32;25;39m123\n") ;  // ESC [pn m , Hello,world红色，其余为绿色。 

    printf("\033[2J\n") ; // 实现屏幕的清空


    // gets() ; 
    printf("Hello from shell\n") ; 
    printf("os version:%s\n" , "1.0.0") ;
    printf("%d %d %d\n" , 1 , 2 , 3 ) ;  

    
    for(int i = 0 ; i < argc  ;  i ++ ) {
        printf("arg: %s\n" , argv[i] ) ; 
    }

    // fork() ; 
    // 子进程直接将运行权交出去，父进程继续运行
    yield() ;  
    for (;;) { 
        printf("shell pid=%d\n" , getpid() ) ; 
        msleep(1000);
    }

    
}