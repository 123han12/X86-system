#include "dev/kbd.h"
#include "tools/log.h"
#include "common/cpu_instr.h"
#include "tools/klib.h"

void exception_handler_kbd(void) ; 


static kbd_state_t kbd_state ; 
static const key_map_t map_table[256] = {
        [0x2] = {'1', '!'},
        [0x3] = {'2', '@'},
        [0x4] = {'3', '#'},
        [0x5] = {'4', '$'},
        [0x6] = {'5', '%'},
        [0x7] = {'6', '^'},
        [0x08] = {'7', '&'},
        [0x09] = {'8', '*' },
        [0x0A] = {'9', '('},
        [0x0B] = {'0', ')'},
        [0x0C] = {'-', '_'},
        [0x0D] = {'=', '+'},
        [0x0E] = {'\b', '\b'},
        [0x0F] = {'\t', '\t'},
        [0x10] = {'q', 'Q'},
        [0x11] = {'w', 'W'},
        [0x12] = {'e', 'E'},
        [0x13] = {'r', 'R'},
        [0x14] = {'t', 'T'},
        [0x15] = {'y', 'Y'},
        [0x16] = {'u', 'U'},
        [0x17] = {'i', 'I'},
        [0x18] = {'o', 'O'},
        [0x19] = {'p', 'P'},
        [0x1A] = {'[', '{'},
        [0x1B] = {']', '}'},
        [0x1C] = {'\n', '\n'},
        [0x1E] = {'a', 'A'},
        [0x1F] = {'s', 'B'},
        [0x20] = {'d',  'D'},
        [0x21] = {'f', 'F'},
        [0x22] = {'g', 'G'},
        [0x23] = {'h', 'H'},
        [0x24] = {'j', 'J'},
        [0x25] = {'k', 'K'},
        [0x26] = {'l', 'L'},
        [0x27] = {';', ':'},
        [0x28] = {'\'', '"'},
        [0x29] = {'`', '~'},
        [0x2B] = {'\\', '|'},
        [0x2C] = {'z', 'Z'},
        [0x2D] = {'x', 'X'},
        [0x2E] = {'c', 'C'},
        [0x2F] = {'v', 'V'},
        [0x30] = {'b', 'B'},
        [0x31] = {'n', 'N'},
        [0x32] = {'m', 'M'},
        [0x33] = {',', '<'},
        [0x34] = {'.', '>'},
        [0x35] = {'/', '?'},
        [0x39] = {' ', ' '},
};


void kbd_init(void) {

    kernel_memset(&kbd_state , 0 , sizeof(kbd_state) ) ; 
    // 这里对于键盘的话不需要过多的初始化，只需要注册一下函数即可。
    irq_install(IRQ1_KEYBOARD , (irq_handler_t)exception_handler_kbd ) ;     
    irq_enable(IRQ1_KEYBOARD) ; 
}

static inline int is_make_code(uint8_t raw_code) {
    return !(raw_code & 0x80 ) ;  
}

static inline char get_key(uint8_t raw_code) {
    return raw_code & 0x7F ; 
}

static void do_normal_key(uint8_t raw_code ) {
    char key = get_key(raw_code) ; 
    int is_make = is_make_code(raw_code) ; 

    switch(key){
        case KEY_RSHEFT:  
            kbd_state.rshift_press = is_make ? 1 : 0 ; 
            break ; 
        case KEY_LSHEFT: 
            kbd_state.lshift_press = is_make ? 1 : 0 ; 
            break ; 
        case KEY_CAPS:
            if(is_make) {
                kbd_state.caps_lock = ~kbd_state.caps_lock ; 
            }
            break ;
        default:
            if(is_make) {
                
                if(kbd_state.lshift_press || kbd_state.rshift_press ) {
                    key = map_table[key].func ; 
                }else {
                    key = map_table[key].normal ; 
                }

                // shift 未按下，'a' -> 'A' 
                if(kbd_state.caps_lock ) {
                    if(key >= 'a' && key <= 'z' ) {
                        key = key - 'a' + 'A' ; 
                    } else if(key >= 'A' && key <= 'Z' ) {
                        key = key - 'A' + 'a' ; 
                    }   
                }
                log_printf("key: %c" , key) ; 
            }
            break ; 
    }
}


void do_handler_kbd(exception_frame_t* frame ){
    uint32_t status = inb(KDB_PORT_STAT) ; 
    if(!(status & KBD_STAT_RECV_READY ) ) {
        pic_send_eoi(IRQ1_KEYBOARD) ; 
        return ;    
    }

    uint8_t raw_code = inb(KDB_PORT_DATA) ; 

    do_normal_key(raw_code) ; 

    // log_printf("key: %c" , key ) ; 


    pic_send_eoi(IRQ1_KEYBOARD) ; 

} 