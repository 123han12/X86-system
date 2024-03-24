#ifndef KBD_H 
#define KBD_H 

#include "cpu/irq.h"
#include "common/types.h"

#define KDB_PORT_DATA     0x60 
#define KDB_PORT_STAT     0x64 
#define KDB_PORT_CMD      0x64 
#define KBD_STAT_RECV_READY    (1 << 0 )

#define KEY_RSHEFT         0x36 
#define KEY_LSHEFT         0x2A

#define KEY_CAPS          0x3A 



typedef struct _key_map_t {
	uint8_t normal;				// 普通功能
	uint8_t func;				// 第二功能
}key_map_t;


/**
 * 状态指示灯
 */
typedef struct _kbd_state_t {
    int caps_lock : 1 ; 
    int lshift_press : 1;       // 左shift按下
    int rshift_press : 1;       // 右shift按下
}kbd_state_t;

void kbd_init(void) ; 
void do_handler_kbd(exception_frame_t* frame ) ; 

#endif 