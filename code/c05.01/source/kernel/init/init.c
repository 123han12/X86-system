﻿/**
 * 内核初始化以及测试代码
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include "comm/boot_info.h"
#include "cpu/cpu.h"

/**
 * 内核入口
 */
void kernel_init (boot_info_t * boot_info) {
    // 初始化CPU，再重新加载
    cpu_init();

    for (;;) {}
}
