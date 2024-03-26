#include "dev/console.h"
#include "tools/klib.h"
#include "common/cpu_instr.h"
#include "dev/tty.h" 

#define CONSOLE_NR            8
static console_t console_buf[CONSOLE_NR];

// 读取
static int read_cursor_pos(void)
{
    int pos;
    // 读取光标的位置
    outb(0x3D4, 0xF);
    pos = inb(0x3d5);
    outb(0x3D4, 0xE);
    pos |= (inb(0x3d5) << 8);
    return pos;
}

static int update_cursor_pos(console_t *console)
{
    // 获取到当前控制台的光标应该处于的位置
    uint16_t pos = console->cursor_row * console->display_cols + console->cursor_col;

    outb(0x3D4, 0xF);
    outb(0x3d5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0xE);
    outb(0x3d5, (uint8_t)((pos >> 8) & 0xFF));

    return pos;
}

// 对[startline , endline]之间的行进行清空
static void erase_rows(console_t *console, int startline, int endline)
{
    disp_char_t *start = console->disp_base + startline * console->display_cols;
    disp_char_t *end = console->disp_base + console->display_cols * (endline + 1);
    while (start < end)
    {
        start->v = ' ';
        start->background = console->background;
        start->foreground = console->foreground;

        start++;
    }
}

// 将console显示区域向上滚动lines行
static void scroll_up(console_t *console, int lines)
{
    disp_char_t *dest = console->disp_base;
    disp_char_t *src = console->disp_base + lines * console->display_cols;
    uint32_t size = (console->display_rows - lines) * console->display_cols * sizeof(disp_char_t);

    kernel_memcpy((void *)dest, (void *)src, size);

    // 上滚几行就将最下面的几行进行清空 [left , right] 两个边界都是闭合的
    erase_rows(console, console->display_rows - lines, console->display_rows - 1);
    console->cursor_row -= lines;
}

static void move_to_col0(console_t *console)
{
    console->cursor_col = 0;
    return;
}

// 将光标的位置移动count位
static void move_forward(console_t *console, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (++console->cursor_col >= console->display_cols)
        {
            // 这里的逻辑并不完善，暂时这样处理
            console->cursor_row++;
            console->cursor_col = 0;

            // 如果当前行的行号大于等于最大的行号，这里需要向上滚动一行
            if (console->cursor_row >= console->display_rows)
            {
                scroll_up(console, 1);
            }
        }
    }
}

static void show_char(console_t *console, char c)
{

    int offset = console->cursor_col + console->cursor_row * console->display_cols;
    disp_char_t *ptr = console->disp_base + offset;

    ptr->c = c;
    ptr->foreground = console->foreground;
    ptr->background = console->background;
    move_forward(console, 1);
}

static void clear_display(console_t *console)
{
    int size = console->display_cols * console->display_rows;

    disp_char_t *start = console->disp_base;
    for (int i = 0; i < size; i++, start++)
    {
        start->c = ' ';
        start->background = console->background;
        start->foreground = console->foreground;
    }
}

/// @brief 如果当前行已经是最后一行了，则需要向上滚动一行
/// @param console
static void move_next_line(console_t *console)
{
    console->cursor_row++;
    if (console->cursor_row >= console->display_rows)
    {
        scroll_up(console, 1); // 向上滚动
    }
}

// 对所有的屏幕进行初始化
int console_init(int idx)
{
    console_t *console = console_buf + idx;
    console->disp_base = (disp_char_t *)CONSOLE_DISP_ADDR + (CONSOLE_ROW_MAX * CONSOLE_COL_MAX) * idx;
    console->display_rows = CONSOLE_ROW_MAX;
    console->display_cols = CONSOLE_COL_MAX;

    // 设置初始光标位置

    if(idx == 0 ) {
        int cursor_pos = read_cursor_pos();
        console->cursor_row = cursor_pos / console->display_cols;
        console->cursor_col = cursor_pos % console->display_cols;
    }else {
        console->cursor_col = 0 ; 
        console->cursor_row = 0 ; 
        clear_display(console) ;    //对屏幕进行清空
        update_cursor_pos(console) ; 
    }

    // 设置颜色
    console->foreground = COLOR_White;
    console->background = COLOR_Block;

    console->old_cursor_col = 0;
    console->old_cursor_row = 0;
    console->write_state = CONSOLE_WRITE_NORMAL;
    console->curr_parm_index = 0;
    return 0;
}

// 将光标向左移动count位
static int move_backword(console_t *console, int count)
{
    int status = -1;
    for (int i = 0; i < count; i++)
    {
        if (console->cursor_col > 0)
        {
            console->cursor_col--;
            status = 0;
        }
        else if (console->cursor_row > 0)
        {
            console->cursor_row--;
            console->cursor_col = console->display_cols - 1;
            status = 0;
        }
        else
        {
            status = -1;
        }
    }
    return status;
}

static void erase_backword(console_t *console)
{
    if (move_backword(console, 1) == 0)
    {
        show_char(console, ' ');
        move_backword(console, 1);
    }
}

static void write_normal(console_t *c, char ch)
{
    switch (ch)
    {
    case ASCII_ESC:
        c->write_state = CONSOLE_WRITE_ESC;
        break;
    case '\n':
        move_next_line(c);
        break;
    case 0x7F:
        erase_backword(c);
        break;
    case '\r':
        move_to_col0(c);
        break;
    case '\b':
        move_backword(c, 1);
        break;
    default:
        if (ch >= ' ' && ch <= '~')
        {
            show_char(c, ch);
        }
        break;
    }
}

static void save_cursor(console_t *console)
{
    console->old_cursor_col = console->cursor_col;
    console->old_cursor_row = console->cursor_row;
}

static void restore_cursor(console_t *console)
{
    console->cursor_col = console->old_cursor_col;
    console->cursor_row = console->old_cursor_row;
}

static void clear_esc_param(console_t *console)
{
    kernel_memset(console->esc_param, 0, sizeof(console->esc_param));
    console->curr_parm_index = 0;
}

static void write_esc(console_t *console, char ch)
{
    switch (ch)
    {
    case '7':
        save_cursor(console);
        console->write_state = CONSOLE_WRITE_NORMAL;
        break;
    case '8':
        restore_cursor(console);
        console->write_state = CONSOLE_WRITE_NORMAL;
        break;
    case '[':
        clear_esc_param(console);
        console->write_state = CONSOLE_WRITE_SQUARE;
        break;
    default:
        console->write_state = CONSOLE_WRITE_NORMAL;
        break;
    }
}

// 参数列表中是设置颜色和字体的
static void set_font_style(console_t *console)
{
    static const color_t color_table[] = {
        COLOR_Block, COLOR_Red, COLOR_Green, COLOR_Yellow,
        COLOR_Blue, COLOR_Magenta, COLOR_Cyan, COLOR_White};

    for (int i = 0; i <= console->curr_parm_index; i++)
    {
        int param = console->esc_param[i];
        if ((param >= 30 && param <= 37))
        {
            console->foreground = color_table[param - 30];
        }
        else if ((param >= 40 && param <= 47))
        {
            console->background = color_table[param - 40];
        }
        else if (param == 39)
        {
            console->foreground = COLOR_White;
        }
        else if (param == 49)
        {
            console->background = COLOR_Block;
        }
    }
}

static void erase_in_display(console_t *console)
{
    // 这里只针对参数为2的情况
    if (console->curr_parm_index < 0)
    {
        return;
    }
    int param = console->esc_param[0];
    if (param == 2)
    {
        erase_rows(console, 0, console->display_rows);
        console->cursor_col = console->cursor_row = 0;
    }
}

static void move_cursor(console_t *console)
{
    console->cursor_row = console->esc_param[0];
    console->cursor_col = console->esc_param[1];
}

// 如果向左移不动了，就停在0
static void move_left(console_t *console, int count)
{
    if (count == 0)
        count = 1;
    int col = console->cursor_col - count;
    console->cursor_col = (col >= 0) ? col : 0;
}

static void move_right(console_t *console, int count)
{
    if (count == 0)
        count = 1;
    int col = console->cursor_col + count;
    console->cursor_col = (col >= console->display_cols) ? console->display_cols - 1 : col;
}

// 处理完了 esc 和 [ 了。
static void write_esc_square(console_t *console, char ch)
{
    if ((ch >= '0' && ch <= '9'))
    {
        int *param = &console->esc_param[console->curr_parm_index];
        *param = (*param * 10) + ch - '0';
    }
    else if ((ch == ';') && (console->curr_parm_index < ESC_PARAM_MAX))
    {
        console->curr_parm_index++;
    }
    else
    {
        switch (ch)
        {
        case 'm':
            set_font_style(console);
            break;
        case 'C':
            move_right(console, console->esc_param[0]);
            break;
        case 'D':
            move_left(console, console->esc_param[0]);
            break;
        case 'H':
        case 'f':
            move_cursor(console);
            break;
        case 'J':
            erase_in_display(console);
            break;
        default:
            break;
        }
        console->write_state = CONSOLE_WRITE_NORMAL;
    }
}
//int console, char *data, int size
int console_write(tty_t* tty )
{

    console_t *c = console_buf + tty->console_idx ; 
    int len = 0;
    do{
        char ch ; 
        int err = tty_fifo_get(&tty->ofifo , &ch ) ; 
        if(err < 0 ) {
            break ; 
        }  
        sem_notify(&tty->osem ) ; 
        switch (c->write_state)
        {
        case CONSOLE_WRITE_NORMAL:
            write_normal(c, ch);
            break;
        case CONSOLE_WRITE_ESC:
            write_esc(c, ch);
            break;
        case CONSOLE_WRITE_SQUARE:
            write_esc_square(c, ch);
            break;
        default:
            break;
        }

        len ++ ;     
    }while(1) ; 
    update_cursor_pos(c);
    return len;
}

void console_close(int console)
{
    return;
}
