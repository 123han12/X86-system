/**
 * 简单的命令行解释器
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include "lib_syscall.h"

int main (int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        print_msg("arg: %s", (int)argv[i]);
    }
    
    // 创建一个自己的副本
    fork();

    yield();

    for (;;) {
        print_msg("pid=%d", getpid());
        msleep(1000);
    }
}