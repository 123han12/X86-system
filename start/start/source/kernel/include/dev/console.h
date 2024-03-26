#ifndef CONSOLE_H 
#define CONSOLE_H 
#include "common/types.h" 
#include "dev/tty.h"

#define CONSOLE_DISP_ADDR    0xb8000
#define CONSOLE_DISP_END     (0xb8000 + 32 * 1024) 
#define CONSOLE_ROW_MAX      25
#define CONSOLE_COL_MAX      80 

#define ASCII_ESC           0x1b 
#define ESC_PARAM_MAX       10 




typedef union _disp_char_t {
    uint16_t v ; 
    struct {
        char c ; 
        char foreground : 4 ; 
        char background : 3 ; 
    } ; 
} disp_char_t ; 


typedef enum {
    COLOR_Block = 0 , 
    COLOR_Blue , 
    COLOR_Green , 
    COLOR_Cyan , 
    COLOR_Red , 
    COLOR_Magenta , 
    COLOR_Brown , 
    COLOR_Gray , 
    COLOR_DarkGray , 
    COLOR_Light_Blue , 
    COLOR_Light_Green , 
    COLOR_Light_Cyan , 
    COLOR_Light_Red , 
    COLOR_Light_Magenta , 
    COLOR_Yellow , 
    COLOR_White , 
}color_t ; 

typedef struct _console_t {

    enum {
        CONSOLE_WRITE_NORMAL , 
        CONSOLE_WRITE_ESC , 
        CONSOLE_WRITE_SQUARE , 
    }write_state ; 


    disp_char_t* disp_base ; 
    int display_rows , display_cols ; 
    int cursor_row , cursor_col ; 

    color_t foreground , background ; 

    int old_cursor_col , old_cursor_row ; 

    int esc_param[ESC_PARAM_MAX] ; 
    int curr_parm_index ; 

} console_t ; 


int console_init(int idx ) ; 

int console_write(tty_t* tty  ); 
void console_close(int console ) ;  

#endif 
