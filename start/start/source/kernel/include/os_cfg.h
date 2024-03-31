#ifndef OS_CFG_H 
#define OS_CFG_H

#define GDT_TABLE_SIZE 256 

#define KERNEL_SELECTOR_CS (1 * 8)
#define KERNEL_SELECTOR_DS (2 * 8)
#define SELECTOR_SYSCALL    (3 * 8) 
#define KERNEL_STACK_SIZE  (8 * 1024)  // 内核栈

#define OS_TICK_MS              10       	// 每毫秒的时钟数

#define OS_VERSION            "22.04"
#define IDLE_TASK_SIZE         1024

#define TASK_NR                128 

#define ROOT_DEV               DEV_DISK , 0xb1

#endif