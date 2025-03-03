#include <oak/io.h>
#include <oak/string.h>
#include <x86/vga.h>

static void correct_addr(u32 *addr) {
    if (*addr % 2 != 0) {
        *addr -= 1;
    }

    if (*addr > VGA_TEXT_MEM_END) {
        *addr = VGA_TEXT_MEM_BASE +
                ((*addr - VGA_TEXT_MEM_END) % VGA_TEXT_MEM_SIZE);
    }
    if (*addr < VGA_TEXT_MEM_BASE) {
        *addr = VGA_TEXT_MEM_SIZE -
                ((VGA_TEXT_MEM_BASE - *addr) % VGA_TEXT_MEM_SIZE);
    }
}

/**
 *  @brief  获取当前屏幕的起始位置
 *  @return  屏幕起始位置地址
 *
 *  从寄存器得到的是 VGA 字符相对于 VGA 基地址的偏移量，
 *  乘 2 后才是实际字节数相对于基地址的偏移量，最后返回值还需加上基地址。
 */
u32 vga_get_screen_start_addr() {
    u32 scr_addr = 0;
    outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    scr_addr = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    scr_addr |= inb(CRT_DATA_REG);

    scr_addr <<= 1;
    return scr_addr + VGA_TEXT_MEM_BASE;
}

/**
 *  @brief  设置屏幕起始位置
 *  @param  scr_addr  屏幕起始位置
 */
u32 vga_set_screen_start_addr(u32 scr_addr) {
    correct_addr(&scr_addr);
    outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    outb(CRT_DATA_REG, ((scr_addr - VGA_TEXT_MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    outb(CRT_DATA_REG, ((scr_addr - VGA_TEXT_MEM_BASE) >> 1) & 0xff);
    return scr_addr;
}

/**
 *  @brief  获取光标的位置
 *  @return  光标地址
 *
 *  从寄存器得到的是 VGA 字符相对于 VGA 基地址的偏移量，
 *  乘 2 后才是实际字节数相对于基地址的偏移量，最后返回值还需加上基地址。
 */
u32 vga_get_cursor_addr() {
    u32 cursor_addr = 0;
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    cursor_addr = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    cursor_addr |= inb(CRT_DATA_REG);

    cursor_addr <<= 1;
    return cursor_addr + VGA_TEXT_MEM_BASE;
}

/**
 *  @brief  设置屏幕起始位置
 *  @param  scr_addr  屏幕起始位置
 */
u32 vga_set_cursor_addr(u32 cursor_addr) {
    correct_addr(&cursor_addr);
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    outb(CRT_DATA_REG, ((cursor_addr - VGA_TEXT_MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    outb(CRT_DATA_REG, ((cursor_addr - VGA_TEXT_MEM_BASE) >> 1) & 0xff);
    return cursor_addr;
}

/**
 *  @brief  清屏
 *
 *  重置屏幕起始位置、光标位置为基地址，将显存所有 VGA 字符设为 0x0720。
 */
void vga_text_clear_screen() {
    vga_set_screen_start_addr(VGA_TEXT_MEM_BASE);
    vga_set_cursor_addr(VGA_TEXT_MEM_BASE);

    u16 *ptr = (u16 *)VGA_TEXT_MEM_BASE;
    while (ptr < (u16 *)(VGA_TEXT_MEM_BASE + VGA_TEXT_MEM_SIZE)) {
        *ptr++ = VGA_ERASE_CHAR;
    }
}

/**
 *  @brief  设置背景色、前景色。
 *  @param  fg  前景色
 *  @param  bg  背景色
 *  @return 带字节属性的空 VGA 字符
 */
vga_attributes vga_text_set_theme(vga_attributes fg, vga_attributes bg) {
    return (bg << 4 | fg) << 8;
}

/**
 *  @brief  在特定位置写 VGA 字符
 *  @param  addr  要写入的内存地址
 *  @param  char  要写入的字符
 *  @param  attr 字符属性
 *  @return 返回实际写入的内存地址
 *
 *  因为地址可能被修正，所以实际返回的地址不一定与原来相同。
 *  该函数只负责写入字符，不修改光标地址。
 */
u32 vga_text_write_char(u32 addr, vga_attributes attr, char ch) {
    correct_addr(&addr);
    u16 *ptr = (u16 *)addr;
    *ptr = (u16)((attr & 0xff00) | ch);
    return addr;
}

/**
 *  @brief  在特定位置写 VGA 字符串
 *  @param  addr  要写入的内存地址
 *  @param  str  要写入的字符串指针
 *  @param  attr 字符属性
 *  @param  count 要写入的字符数
 *  @return 返回实际写入的内存地址
 *
 *  因为地址可能被修正，所以实际返回的地址不一定与原来相同。
 *  该函数只负责写入字符，不修改光标地址。
 */
u32 vga_text_write_str(u32 addr, vga_attributes attr, const char *str,
                       u32 count) {
    correct_addr(&addr);
    u16 *ptr = (u16 *)addr;

    while (count--) {
        *ptr = (u16)((attr & 0xff00) | *str);
        str++;
        ptr++;
    }

    return addr;
}

/**
 *  @brief  向上滚动屏幕一行
 *  @return 返回新的屏幕起始位置地址
 *
 *  当显示内存不足以再增加一行，即当前屏幕起始位置地址、屏幕最大字节数和每行字
 *  节数之和大于显示内存最大值时，将当前屏幕内容拷贝到显示内存起始地址，也就是
 *  回环使用显示内存。
 *
 *  然后，和一般情况相同，屏幕起始位置地址增加一行并清空增加的一行。
 *  该函数不移动光标。
 */
u32 vga_text_scroll_up() {
    u32 scr_addr = vga_get_screen_start_addr();

    if (scr_addr + VGA_SCR_SIZE + VGA_ROW_SIZE >= VGA_TEXT_MEM_END) {
        memcpy((void *)VGA_TEXT_MEM_BASE, (void *)scr_addr, VGA_SCR_SIZE);
        vga_set_screen_start_addr(VGA_TEXT_MEM_BASE);
        scr_addr = VGA_TEXT_MEM_BASE;
    }

    u16 *ptr = (u16 *)(scr_addr + VGA_SCR_SIZE);
    for (size_t i = 0; i < VGA_TEXT_WIDTH; i++) {
        *ptr++ = VGA_ERASE_CHAR;
    }
    vga_set_screen_start_addr(scr_addr + VGA_ROW_SIZE);

    return scr_addr + VGA_ROW_SIZE;
}
