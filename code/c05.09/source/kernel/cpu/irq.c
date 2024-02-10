/**
 * 中断处理
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"

#define IDT_TABLE_NR			128				// IDT表项数量

static gate_desc_t idt_table[IDT_TABLE_NR];	// 中断描述表

static void do_default_handler (exception_frame_t * frame, const char * message) {
    for (;;) {}
}

void do_handler_unknown (exception_frame_t * frame) {
	do_default_handler(frame, "Unknown exception.");
}

void do_handler_divider(exception_frame_t * frame) {
	do_default_handler(frame, "Device Error.");
}

/**
 * @brief 中断和异常初始化
 */
void irq_init(void) {	
	for (uint32_t i = 0; i < IDT_TABLE_NR; i++) {
    	gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS, (uint32_t) exception_handler_unknown,
                  GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);
	}

	// 设置异常处理接口
	irq_install(IRQ0_DE, (irq_handler_t)exception_handler_divider);

	lidt((uint32_t)idt_table, sizeof(idt_table));
}

/**
 * @brief 安装中断或异常处理程序
 */
int irq_install(int irq_num, irq_handler_t handler) {
	if (irq_num >= IDT_TABLE_NR) {
		return -1;
	}

    gate_desc_set(idt_table + irq_num, KERNEL_SELECTOR_CS, (uint32_t) handler,
                  GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);
	return 0;
}