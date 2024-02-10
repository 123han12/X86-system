/**
 * 自己动手写操作系统
 *
 * 二级加载部分，用于实现更为复杂的初始化、内核加载的工作。
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#ifndef LOADER_H
#define LOADER_H

#include "comm/types.h"
#include "comm/boot_info.h"

// 内存检测信息结构
typedef struct SMAP_entry {
    uint32_t BaseL; // base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; // length uint64_t
    uint32_t LengthH;
    uint32_t Type; // entry Type
    uint32_t ACPI; // extended
}__attribute__((packed)) SMAP_entry_t;

#endif // LOADER_H
