/**
 * 功能：公共头文件
 *
 *创建时间：2022年8月31日
 *作者：李述铜
 *联系邮箱: 527676163@qq.com
 *相关信息：此工程为《从0写x86 Linux操作系统》的前置课程，用于帮助预先建立对32位x86体系结构的理解。整体代码量不到200行（不算注释）
 *课程请见：https://study.163.com/course/introduction.htm?courseId=1212765805&_trace_c_p_k2_=0bdf1e7edda543a8b9a0ad73b5100990
 */
#ifndef OS_H
#define OS_H

#define KERNEL_CODE_SEG  8 
#define KERNEL_DATA_SEG  16 
#define APP_CODE_SEG   (24 | 3 )
#define APP_DATA_SEG   (32 | 3 )  
#define TASK0_TSS_SEG  40 
#define TASK1_TSS_SEG  48 
#define SYSCALL_SEG    56 
#define TASK0_LDT_SEG  64 
#define TASK1_LDT_SEG  72 



#define TASK_CODE_SEG (0 * 8 | 0x4 | 3)
#define TASK_DATA_SEG (1 * 8 | 0x4 | 3)

#endif // OS_H



