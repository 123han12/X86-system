

	.text
	.global _start 
	.extern kernel_init
_start: 



	# 假设boot_info_ *  指针存放在 eax 中
	push %ebp 
	mov %esp , %ebp 
	mov 0x8(%ebp) , %eax 

	# 这里进行参数的压栈
	push %eax   
	call kernel_init
	jmp . 