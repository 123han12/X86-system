#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "common/cpu_instr.h"
#include "os_cfg.h"
#include "tools/log.h"
#include "common/types.h"
#include "core/task.h"


static gate_desc_t idt_table[IDT_TABLE_NR] ; 

static void dump_core_regs(exception_frame_t* frame)
{
	uint32_t ss , esp ; 
	if(frame->cs & 0x3 ) {
		ss = frame->ss3 ; 
		esp = frame->esp3 ; 
	}else {
		ss = frame->ds ; 
		esp = frame->esp ; 
	}

	log_printf("IRQ: %d  error code: %d " , frame->num , frame->error_code ) ; 
	log_printf("CS: %d\r\nDS: %d\r\nES: %d\r\nSS: %d\r\nFS:%d\r\nGS:%d",
               frame->cs, frame->ds, frame->es, ss , frame->fs, frame->gs  
    );
    log_printf("EAX:0x%x\r\n"
                "EBX:0x%x\r\n"
                "ECX:0x%x\r\n"
                "EDX:0x%x\r\n"
                "EDI:0x%x\r\n"
                "ESI:0x%x\r\n"
                "EBP:0x%x\r\n"
                "ESP:0x%x\r\n",
               frame->eax, frame->ebx, frame->ecx, frame->edx,
               frame->edi, frame->esi, frame->ebp,  esp ) ;
    log_printf("EIP:0x%x\r\nEFLAGS:0x%x\r\r\n", frame->eip, frame->eflags);
}

static void do_default_handler(exception_frame_t* frame , const char* message )
{
	log_printf("---------------------------------") ; 
	log_printf("IRQ/Exception happend: %s." , message) ; 

	dump_core_regs(frame) ; 


	log_printf("--------------------------------") ; 
	if(frame->cs & 0x3 ) { // 如果是在用户特权级
		sys_exit(frame->error_code) ; // 直接退出
	}else {
		while(1){
			hlt() ; 
		}
	}
}

void do_handler_unknown (exception_frame_t * frame) {
	do_default_handler(frame, "Unknown exception.");
}

void do_handler_divider(exception_frame_t * frame) {
	do_default_handler(frame, "Device exception......");
}

void do_handler_Debug(exception_frame_t * frame) {
	do_default_handler(frame, "Debug Exception");
}

void do_handler_NMI(exception_frame_t * frame) {
	do_default_handler(frame, "NMI Interrupt.");
}

void do_handler_breakpoint(exception_frame_t * frame) {
	do_default_handler(frame, "Breakpoint.");
}

void do_handler_overflow(exception_frame_t * frame) {
	do_default_handler(frame, "Overflow.");
}

void do_handler_bound_range(exception_frame_t * frame) {
	do_default_handler(frame, "BOUND Range Exceeded.");
}

void do_handler_invalid_opcode(exception_frame_t * frame) {
	do_default_handler(frame, "Invalid Opcode.");
}

void do_handler_device_unavailable(exception_frame_t * frame) {
	do_default_handler(frame, "Device Not Available.");
}

void do_handler_double_fault(exception_frame_t * frame) {
	do_default_handler(frame, "Double Fault.");
}

void do_handler_invalid_tss(exception_frame_t * frame) {
	do_default_handler(frame, "Invalid TSS");
}

void do_handler_segment_not_present(exception_frame_t * frame) {
	do_default_handler(frame, "Segment Not Present.");
}

void do_handler_stack_segment_fault(exception_frame_t * frame) {
	do_default_handler(frame, "Stack-Segment Fault.");
}

void do_handler_general_protection(exception_frame_t * frame) {
	log_printf("---------") ; 
	log_printf("general_protection fault....") ; 

	if(frame->error_code & ERR_EXT ) 
	{
		log_printf("The exception occured during delivery of an event external to the program.:0x%x" , read_cr2() ) ; 
	}else {
		log_printf("The exception occured during delivery of s software interrupt:0x%x" , read_cr2() ) ; 
	}

	if(frame->error_code & ERR_IDT )
	{
		log_printf("The index portion of the error code refers to a gate descriptor in the IDT:0x%x" , read_cr2() ) ; 
	} else {
		log_printf("The index portion refers to a descriptor in the GDT or the current LDT:0x%x" , read_cr2() ) ; 
	}

	log_printf("selector index:%d" , frame->error_code & 0xFFF8) ; 

	dump_core_regs(frame) ; 
		
	if(frame->cs & 0x3 ) { // 如果是在用户特权级
		sys_exit(frame->error_code) ; // 直接退出
	}else {
		while(1){
			hlt() ; 
		}
	}


}

void do_handler_page_fault(exception_frame_t * frame) {

	log_printf("---------") ; 
	log_printf("Page fault....") ; 

	if(frame->error_code & ERR_PAGE_P )
	{
		log_printf("The fault was cased by a page-level protection violation.:0x%x" , read_cr2() ) ; 
	}else {
		log_printf("The fault was caused by a non-present page.:0x%x" , read_cr2() ) ; 
	}

	if(frame->error_code & ERR_PAGE_W )
	{
		log_printf("The access causing the fault was a write:0x%x" , read_cr2() ) ; 
	} else {
		log_printf("The access causing the fault was a read:0x%x" , read_cr2() ) ; 
	}

	if(frame->error_code & ERR_PAGE_US ) 
	{
		log_printf("A user-mode access caused the fault:0x%x" , read_cr2() ) ; 
	} else {
		log_printf("A supervsior-mode access the fault:0x%x" , read_cr2() ) ; 
	}

	dump_core_regs(frame) ; 
	
	if(frame->cs & 0x3 ) { // 如果是在用户特权级
		sys_exit(frame->error_code) ; // 直接退出
	}else {
		while(1){
			hlt() ; 
		}
	}
}

void do_handler_fpu_error(exception_frame_t * frame) {
	do_default_handler(frame, "X87 FPU Floating Point Error.");
}

void do_handler_alignment_check(exception_frame_t * frame) {
	do_default_handler(frame, "Alignment Check.");
}

void do_handler_machine_check(exception_frame_t * frame) {
	do_default_handler(frame, "Machine Check.");
}

void do_handler_smd_exception(exception_frame_t * frame) {
	do_default_handler(frame, "SIMD Floating Point Exception.");
}

void do_handler_virtual_exception(exception_frame_t * frame) {
	do_default_handler(frame, "Virtualization Exception.");
}



uint32_t irq_install(uint32_t irq_num , irq_handler_t handler )  
{
    if(irq_num >= IDT_TABLE_NR ) 
    {
        return -1 ; 
    }
    gate_dest_set(idt_table + irq_num , KERNEL_SELECTOR_CS , (uint32_t)handler , 
      GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT   
    ) ; 
} 

// 初始化 8259可编程中断控制器
static void init_pic(void)
{
	outb(PIC0_ICW1 , PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4) ;
	outb(PIC0_ICW2 , IRQ_PIC_START ) ; 
	outb(PIC0_ICW3 , 1 << 2 ) ;
	outb(PIC0_ICW4 , PIC_ICW4_8086 ) ;

	outb(PIC1_ICW1 , PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4) ;
	outb(PIC1_ICW2 , IRQ_PIC_START + 8 ) ; 
	outb(PIC1_ICW3 , 2 ) ; // 告知从片连接的主片的引脚
	outb(PIC1_ICW4 , PIC_ICW4_8086 ) ; 
	
	
	// 设置8259 主片和从片的所有中断屏蔽位都打开
	outb(PIC0_IMR , 0xFF & ~(1 << 2 ) ) ; 
	outb(PIC1_IMR , 0xFF ) ; 
}

void irq_init(void)
{
    // 初始化每一个表项
    for(int i = 0 ; i < IDT_TABLE_NR ; i ++ )
    {
        gate_dest_set(( idt_table + i ) , KERNEL_SELECTOR_CS , (uint32_t)do_handler_unknown , 
            GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT ) ;  
    }

    irq_install(IRQ0_DE, exception_handler_divider);
	irq_install(IRQ1_DB, exception_handler_Debug);
	irq_install(IRQ2_NMI, exception_handler_NMI);
	irq_install(IRQ3_BP, exception_handler_breakpoint);
	irq_install(IRQ4_OF, exception_handler_overflow);
	irq_install(IRQ5_BR, exception_handler_bound_range);
	irq_install(IRQ6_UD, exception_handler_invalid_opcode);
	irq_install(IRQ7_NM, exception_handler_device_unavailable);
	irq_install(IRQ8_DF, exception_handler_double_fault);
	irq_install(IRQ10_TS, exception_handler_invalid_tss);
	irq_install(IRQ11_NP, exception_handler_segment_not_present);
	irq_install(IRQ12_SS, exception_handler_stack_segment_fault);
	irq_install(IRQ13_GP, exception_handler_general_protection);
	irq_install(IRQ14_PF, exception_handler_page_fault);
	irq_install(IRQ16_MF, exception_handler_fpu_error);
	irq_install(IRQ17_AC, exception_handler_alignment_check);
	irq_install(IRQ18_MC, exception_handler_machine_check);
	irq_install(IRQ19_XM, exception_handler_smd_exception);
	irq_install(IRQ20_VE, exception_handler_virtual_exception); 


    // 将 idt_table 地址加载到 idtr 寄存器
    lidt((uint32_t)idt_table , sizeof(idt_table) ) ; 


	init_pic() ; 

}


void irq_disable_global(void)
{
	cli() ; 
}
void irq_enable_global(void)   // 设置flags 中的 iF 中断标志位，使得cpu能够接受外部的中断
{
	sti() ; 
}

void irq_enable(int irq_num)
{
	if(irq_num < IRQ_PIC_START ) return ; 
	irq_num -= IRQ_PIC_START ; 

	if(irq_num < 8 ) 
	{
		uint8_t mask = inb(PIC0_IMR) & ~(1 << irq_num); 
		outb(PIC0_IMR , mask ) ; 
	}
	else {
		irq_num -= 8 ; 
		uint8_t mask = inb(PIC1_IMR) & ~(1 << irq_num ); 
		outb(PIC1_IMR , mask) ; 
	}
}

void irq_disable(int irq_num)
{
	if(irq_num < IRQ_PIC_START ) return ; 
	irq_num -= IRQ_PIC_START ; 
	if(irq_num < 8 )
	{
		uint8_t mask = inb(PIC0_IMR) | (1 << irq_num) ; 
		outb(PIC0_IMR , mask ) ; 
	}
	else {
		irq_num -= 8 ; 
		uint8_t mask = inb(PIC1_IMR) | (1 << irq_num) ; 
		outb(PIC0_IMR , mask ) ; 
	}
}

void pic_send_eoi(int irq_num)
{
	irq_num -= IRQ_PIC_START ; 

	if(irq_num >= 8 ) 
	{
		outb(PIC1_OCW2 , PIC_OCW2_EOI ) ; 
	}
	outb(PIC0_OCW2 , PIC_OCW2_EOI) ; 
}


void pannic(const char* filename , int line , const char* func , const char* cond ) 
{
	log_printf("assert failed: %s" , cond ) ; 
	log_printf("file:%s\r\nline:%d\r\nfunc:%s\r\n" , filename , line , func ) ;
	for(;;) {hlt() ;  } 
}


irq_state_t irq_enter_protection(void) 
{
	irq_state_t state = read_eflags() ; 
	irq_disable_global() ; 

	return state ; 
}

void irq_exit_protection(irq_state_t state )
{
	write_eflags(state) ;  
} 