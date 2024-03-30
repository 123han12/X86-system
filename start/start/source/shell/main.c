
#include "lib_syscall.h"
#include <stdio.h>
#include "main.h" 
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/file.h>

static cli_t cli ; 
static const char*  promot  = "sh>>" ; 

/// @brief 展示命令表中的所有命令，并展示其用法
/// @param argc 
/// @param argv 
/// @return 
static int do_help(int argc , char** argv ) {
    const cli_cmd_t* start = cli.cmd_start ; 
    while(start < cli.cmd_end ) {
        printf("%s %s\n" , start->name , start->usage ) ; 
        start ++ ; 
    }

    return 0 ; 
} 
static int do_clear(int argc , char** argv ) {

    printf("%s" , ESC_CLEAR_SCREEN ) ; 
    printf("%s" , ESC_MOVE_CURSOR(0 , 0 ) ) ; 
    return 0 ; 
}

static int do_echo(int argc , char** argv ) {
    if(argc == 1 ) {
        char msg_buf[128] ; 
        fgets(msg_buf , sizeof msg_buf , stdin ) ; 
        msg_buf[sizeof(msg_buf) - 1] = '\0' ; 
        puts(msg_buf) ; 
        return 0 ; 
    }

    int count = 1 ; 
    int ch ; 
    while((ch = getopt(argc , argv , "n:h") ) != -1 ) { // getopt 会自动调整argv中的参数的顺序
        switch (ch)
        {
        case 'h':
            puts("echo --help:") ; 
            puts("Usage: echo [-n count] message") ; 
            optind = 1 ; 
            return 0 ;
        case 'n':
            count = atoi(optarg) ;
            break ; 
        case '?':
            if(optarg ) {
                fprintf(stderr , ESC_COLOR_ERROR"Unknown option: -%s\n"ESC_COLOR_DEFAULT , optarg ); 
            }
            optind = 1 ; 
            return -1 ; 
        default: 
            break;
        }
    }
    if(optind > argc - 1 ) {
        fprintf(stderr , ESC_COLOR_ERROR"Message is empty\n"ESC_COLOR_DEFAULT );
        optind = 1 ; 
        return -1 ;
    }
    char* message = argv[optind] ; 

    for(int i = 0 ; i < count ; i ++ ) {
        puts(message ) ; 
    }
    optind = 1 ; 
    return 0 ; 
}

static int do_exit(int argc , char** argv ) {
    exit(0) ; // 参数表示的是进程退出的状态
    return 0 ; 
}

static const cli_cmd_t cmd_list[] = {
    {
        .name = "help" , 
        .usage = "help -- list supported command" , 
        .do_func = do_help , 
    } , 
    {
        .name = "clear" , 
        .usage = "clear -- clear screen" , 
        .do_func = do_clear , 
    } , 
    {
        .name = "echo" , 
        .usage = "echo [-n count] mssage -- echo something" , 
        .do_func = do_echo , 
    } , 
    {
        .name = "quit" , 
        .usage = "quit from shell." , 
        .do_func = do_exit , 
    } , 

} ; 

static void cli_init(const char* promot , const cli_cmd_t* cmd_list , int size ) {
    cli.promot = promot ; 
    cli.cmd_start = cmd_list ; 
    cli.cmd_end = cmd_list + size ; 
    memset(cli.curr_input , 0 ,  CLI_INPUT_SIZE ) ; 

} 

void show_promot(const char* str ) {
    printf("%s" , str ) ; 
    fflush(stdout) ; // 清空newlib库中的缓存
}

static const cli_cmd_t* find_buitin(const char* name ) {
    for(const cli_cmd_t* cmd = cli.cmd_start ; cmd < cli.cmd_end ; cmd ++ ) {
        if(strcmp(name , cmd->name ) == 0 ) {
            return cmd ;  
        } 
    }
    return (cli_cmd_t*)0 ; 
}

static void run_builtin(const cli_cmd_t* cmd , int argc , char** argv ) {
    int ret = cmd->do_func(argc , argv ) ;
    if(ret < 0 ) {
        fprintf(stderr , ESC_COLOR_ERROR"error: %d\n"ESC_COLOR_DEFAULT , ret ) ; 
    } 
}

static void run_exec_file(const char* path , int argc , char** argv ) {
    int pid = fork() ; 
    if(pid < 0 ) {
        printf("create child process failed....\n") ;

    }else if(pid == 0 ){
        for(int i = 0 ; i < argc ; i ++ ) {
            printf("arg %d = %s \n" , i , argv[i] ) ; 
        }
        exit(-1) ; 
    }else {
        int status ; 
        int pid = wait(&status) ; // 等待子进程退出，并将退出的状态设置到status中,返回值为退出的子进程的id
        fprintf(stderr , "cmd %s result:%d , pid=%d\n" , path , status , pid ) ; 
    }
}


int main (int argc, char **argv) { 
    open(argv[0] , O_RDWR) ; // 0 -> dev0   
    dup(0) ;  // 1 -> dev0 
    dup(0) ;  // 2 -> dev0 

    cli_init(promot , cmd_list , sizeof(cmd_list ) / sizeof(cmd_list[0] ) ); 

    for(;;) {
        show_promot(cli.promot) ; 
        char* str = fgets(cli.curr_input , CLI_INPUT_SIZE , stdin ) ; 
        if(str == NULL ) {
            continue ; 
        }

        char* ptr = strchr(cli.curr_input , '\n'); 
        if(ptr) {
            *ptr = '\0' ; 
        } 
        char* cr = strchr(cli.curr_input , '\r' ); 
        if(cr) {
            *cr = '\0' ; 
        } 
        
        int argc = 0 ; 
        char* argv[CLI_MAX_ARG_COUNT] ; 
        memset(argv , 0 , sizeof(argv) ) ; 
        const char* space = " " ; 
        char* token = strtok(cli.curr_input , space ) ;  // 会将space 都替换为 \0
        
        while(token) {
            argv[argc++] = token ; 
            token = strtok(NULL , space ) ;  // 内部会自动去扫，不需要再去重新传入cli.curr_input了
        }
        if(argc == 0 ) {
            continue ; 
        }
        const cli_cmd_t* cmd = find_buitin(argv[0] ) ; 
        if(cmd ) {
            run_builtin(cmd , argc , argv ) ; 
            continue ; 
        }


        run_exec_file("" , argc , argv ) ; 


        fprintf(stderr , ESC_COLOR_ERROR"Unknown command: %s\n"ESC_COLOR_DEFAULT , cli.curr_input ); 

        
    }


    return 0 ; 
}