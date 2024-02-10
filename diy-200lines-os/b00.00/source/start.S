/**
 * 功能：16位与32位的启动混合代码
 *
 *创建时间：2022年8月31日
 *作者：李述铜
 *联系邮箱: 527676163@qq.com
 *相关信息：此工程为《从0写x86 Linux操作系统》的前置课程，用于帮助预先建立对32位x86体系结构的理解。整体代码量不到200行（不算注释）
 *课程请见：https://study.163.com/course/introduction.htm?courseId=1212765805&_trace_c_p_k2_=0bdf1e7edda543a8b9a0ad73b5100990
 */
	#include "os.h"

	// 声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start , time_int , syscall_handler 

	.extern gdt_table , pg_dir , idt_table , task_0 , task_sched  , do_syscall 
	// 指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
  	.code16
	// 以下是代码区
 	.text
_start: 
	mov $0 , %ax 
	mov %ax , %ds 
	mov %ax , %es
	mov %ax , %ss 
	mov %ax , %gs 
	mov %ax , %fs 
	mov $_start , %esp 

read_self_all:
	mov $_start_32 , %bx 
	mov $0x2 , %cx 
	mov $0x0240 , %ax 
	mov $0x80 , %dx 
	int $0x13 
	jc read_self_all


	# 进入保护模式的固定流程

	cli    # 关中断
	lgdt gdt_desc  # 将 gdt表的基址存放到gdtr寄存器中去 
	lidt idt_desc

	# 将 CR0的D0 位置为1 表示进入保护模式
	mov $1 , %eax 
	lmsw %ax     

	jmp $KERNEL_CODE_SEG , $_start_32   # 设置 cs = 8 , _start_32 为段内偏移地址

	.org 0x1fe 
	.byte 0x55 , 0xaa 

	.code32 
	.text  

_start_32:
	
	# 对各个寄存器进行初始化
	mov $KERNEL_DATA_SEG ,  %ax 
	mov %ax , %ds 
	mov %ax , %es
	mov %ax , %ss 
	mov %ax , %gs 
	mov %ax , %fs 
	mov $_start , %esp



	call os_init   # 调用初始化函数

	# 打开页表机制
	mov $pg_dir , %eax 
	mov %eax , %cr3      # 注意 pg_dir 的地址是4kB的倍数



	
	mov %cr4 , %eax   # 将CR4的D4置位 
	orl $(1 << 4 ) , %eax 
	mov %eax , %cr4  

	mov %cr0 , %eax 
	orl  $(1 << 31) , %eax
	mov %eax , %cr0 
	
	mov $TASK0_TSS_SEG  , %ax 
	ltr %ax            # 将ax中的值放到TR寄存器中


	mov $TASK0_LDT_SEG , %ax 
	lldt %ax  
	 
	push $TASK_DATA_SEG 
	push $task0_dpl3_stack + 1024 * 4 
	push $0x202 
	push $TASK_CODE_SEG
	push $task_0_entry  
	iret 
task_0_entry:               # 配置为特权级3的情况下运行

	mov %ss , %ax 
	mov %ax , %ds 
	mov %ax , %es 
	# 配置栈空间
	jmp task_0 


time_int:

	push %ds
	pusha     # 相当于 push AX , CX , DX , BX , original SP , BP , SI , DI 
	mov $0x20 , %al 
	outb %al , $0x20 


	mov $KERNEL_DATA_SEG , %ax 
	mov %ax , %ds 
	

	call task_sched 

	# 恢复现场
	popa 
	pop %ds 
	iret                  # 必须设置中断返回指令

syscall_handler:	

	push %ds
	pusha     # 相当于 push AX , CX , DX , BX , original SP , BP , SI , DI 

	mov $KERNEL_DATA_SEG , %ax 
	mov %ax , %ds 

	mov %esp , %ebp 

	push 13*4(%ebp) 
	push 12*4(%ebp)
	push 11*4(%ebp) 
	call do_syscall 
	add $(3 * 4) , %esp 
	# 恢复现场
	popa 
	pop %ds 
	retf $(3*4) 

gdt_desc:
	.word (256 * 8 ) - 1   # 界限是其总字节数 - 1 
	.long gdt_table        # 存放gdt_table的起始地址
idt_desc:
	.word (256 * 8 ) - 1   # 界限是其总字节数 - 1 
	.long idt_table        # 存放idt_table的起始地址

