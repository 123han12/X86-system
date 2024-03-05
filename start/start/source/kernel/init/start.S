
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


	.text
	.extern do_handler_unkown
	.global exception_handler_unkown
exception_handler_unkown:

	# 保存现场
	pusha 
	push %ds 
	push %es 
	push %fs 
	push %gs 
	

	call do_handler_unkown

	# 恢复现场
	pop %gs 
	pop %fs 
	pop %es 
	pop %ds 
	popa 
	iret 
