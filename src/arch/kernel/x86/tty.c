#include <oak/tty.h>
#include <oak/types.h>
#include <x86/vga.h>

u32 tty_screen_addr = 0;
u32 tty_cursor_addr = 0;
u32 tty_column = 0;
u32 tty_row = 0;

vga_attributes tty_theme_color = VGA_ERASE_CHAR & 0xff00;

static void update_column_and_row() {
    u32 offset = (tty_cursor_addr - tty_screen_addr) >> 1;
    tty_column = offset % VGA_TEXT_WIDTH;
    tty_row = offset / VGA_TEXT_WIDTH;
}

/**
 *  @brief  删除光标前一个字符
 *
 *  光标向左移动一个 VGA 字符，并删除该字符
 */
static void command_bs() {
    if (tty_cursor_addr > VGA_TEXT_MEM_BASE) {
        tty_cursor_addr -= 2;
        vga_text_write_char(tty_cursor_addr, tty_theme_color, ' ');
        vga_set_cursor_addr(tty_cursor_addr);
        update_column_and_row();
    }
}

/**
 *  @brief  删除光标当前字符
 *
 *  删除光标当前 VGA 字符，但不移动光标
 */
static void command_del() {
    vga_text_write_char(tty_cursor_addr, tty_theme_color, ' ');
}

/**
 *  @brief  回车
 *
 *  将光标移动到行首
 */
static void command_cr() {
    tty_cursor_addr -= (tty_column << 1);
    tty_column = 0;
    vga_set_cursor_addr(tty_cursor_addr);
}

/**
 *  @brief  换行
 *
 *  将光标移动到下一行对应位置
 */
static void command_lf() {
    if (tty_row + 1 < VGA_TEXT_HEIGHT) {
        tty_row++;
        tty_cursor_addr += VGA_ROW_SIZE;
        vga_set_cursor_addr(tty_cursor_addr);
        return;
    }
    tty_scroll_up();
    u32 cursor_offset = VGA_ROW_SIZE * (tty_row) + tty_column * VGA_CHAR_SIZE;
    tty_cursor_addr = vga_set_cursor_addr(tty_screen_addr + cursor_offset);
    update_column_and_row();
}

void tty_scroll_up() { tty_screen_addr = vga_text_scroll_up(); }

/**
 *  @brief  设置背景色、前景色。
 *  @param  fg  前景色
 *  @param  bg  背景色
 */
void tty_set_theme(vga_attributes fg, vga_attributes bg) {
    tty_theme_color = vga_text_set_theme(fg, bg);
}

/**
 *  @brief  向 VGA 显存写入一个字符
 *  @param  chr  要写入的字符
 */
// void tty_put_char(char chr) {}

/**
 *  @brief  向 TTY 写入字符串
 *  @param  buf  字符串指针
 *  @param  count  要写入的字符数量
 */
void tty_write_str(char *buf, u32 count) {
    char ch = 0;
    while (count--) {
        ch = *buf++;
        switch (ch) {
        case ASCII_NUL:
            break;
        case ASCII_ENQ:
            break;
        case ASCII_BEL:
            break;
        case ASCII_BS:
            command_bs();
            break;
        case ASCII_HT:
            break;
        case ASCII_LF:
            command_lf();
            command_cr();
            break;
        case ASCII_VT:
            break;
        case ASCII_FF:
            command_lf();
            break;
        case ASCII_CR:
            command_cr();
            break;
        case ASCII_DEL:
            break;
        default:
            if (tty_cursor_addr + 2 >= tty_screen_addr + VGA_SCR_SIZE) {
                tty_scroll_up();
                u32 cursor_offset =
                    VGA_ROW_SIZE * (tty_row - 1) + tty_column * VGA_CHAR_SIZE;
                tty_cursor_addr =
                    vga_set_cursor_addr(tty_screen_addr + cursor_offset);
            }
            vga_text_write_char(tty_cursor_addr, tty_theme_color, ch);
            tty_cursor_addr = vga_set_cursor_addr(tty_cursor_addr + 2);
            update_column_and_row();
            break;
        }
    }
}

/**
 *  @brief  清屏
 *
 *  屏幕起始位置、光标位置置基地址，将显存所有 VGA 字符设为 0x0720。
 */
void tty_clear() {
    vga_text_clear_screen();
    tty_screen_addr = VGA_TEXT_MEM_BASE;
    tty_cursor_addr = VGA_TEXT_MEM_BASE;
    tty_column = 0;
    tty_row = 0;
}

void tty_init() {
    tty_clear();
    tty_set_theme(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}
