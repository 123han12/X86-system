
#include "os_cfg.h"

	.text
	.global _start 
	.extern kernel_init
	.extern init_main
_start: 

	# 假设boot_info_ *  指针存放在 eax 中
	push %ebp 
	mov %esp , %ebp 
	mov 0x8(%ebp) , %eax 

	# 这里进行参数的压栈
	push %eax   
	call kernel_init

	jmp $KERNEL_SELECTOR_CS , $gdt_reload

gdt_reload:
	mov $KERNEL_SELECTOR_DS , %ax 
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

	mov $(stack + KERNEL_STACK_SIZE) , %esp 

	# 栈和段沿用之前的设置
	jmp init_main  

	.bss
.comm stack , KERNEL_STACK_SIZE 


# 利用宏实现汇编代码的复用
	.text
.macro exception_handler name num with_error_code
	    .extern do_handler_\name
		.global exception_handler_\name
	exception_handler_\name:
		// 如果没有错误码，压入一个缺省值
		// 这样堆栈就和有错误码的情形一样了
		.if \with_error_code == 0
			push $0
		.endif

		// 压入异常号
		push $\num

		// 保存所有寄存器
		pushal
		push %ds
		push %es
		push %fs
		push %gs

		// 调用中断处理函数
		push %esp
		call do_handler_\name
		add $(1*4), %esp		// 丢掉esp

		// 恢复保存的寄存器
		pop %gs
		pop %fs
		pop %es
		pop %ds
		popal

		// 跳过压入的异常号和错误码
		add $(2*4), %esp
		iret
.endm


exception_handler unkown, -1, 0
exception_handler divider, 0, 0