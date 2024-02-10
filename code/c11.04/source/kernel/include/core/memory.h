/**
 * 内存管理
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#ifndef MEMORY_H
#define MEMORY_H

#include "tools/bitmap.h"
#include "comm/boot_info.h"
#include "ipc/mutex.h"

/**
 * @brief 地址分配结构
 */
typedef struct _addr_alloc_t {
    mutex_t mutex;              // 地址分配互斥信号量
    bitmap_t bitmap;            // 辅助分配用的位图

    uint32_t page_size;         // 页大小
    uint32_t start;             // 起始地址
    uint32_t size;              // 地址大小
}addr_alloc_t;

void memory_init (boot_info_t * boot_info);

#endif // MEMORY_H