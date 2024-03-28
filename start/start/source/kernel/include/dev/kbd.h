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

#define KEY_E0              0xE0 
#define KEY_E1              0xE1 

#define KEY_CAPS          0x3A 

#define KEY_CTRL          0x1D 
#define KEY_ALT           0x38


#define KEY_F1            0x3B
#define KEY_F2            0x3C
#define KEY_F3            0x3D
#define KEY_F4            0x3E
#define KEY_F5            0x3F
#define KEY_F6            0x40
#define KEY_F7            0x41
#define KEY_F8            0x42
#define KEY_F9            0x43
#define KEY_F10           0x44
#define KEY_F11           0x57
#define KEY_F12           0x58

#define	ASCII_DEL		0x7F






typedef struct _key_map_t {
	uint8_t normal;				// 普通功能
	uint8_t func;				// 第二功能
}key_map_t;


/**
 * 状态指示灯
 */
typedef struct _kbd_state_t {
    int lalt_press : 1 ; 
    int ralt_press : 1 ; 
    int lctrl_press : 1 ; 
    int rctrl_press : 1 ; 
    int caps_lock : 1 ; 
    int lshift_press : 1;       // 左shift按下
    int rshift_press : 1;       // 右shift按下



}kbd_state_t;

void kbd_init(void) ; 
void do_handler_kbd(exception_frame_t* frame ) ; 

#endif 