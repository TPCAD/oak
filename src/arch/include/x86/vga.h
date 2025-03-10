#ifndef OAK_VGA_H
#define OAK_VGA_H

#include <oak/types.h>

// 每行 80 个 VGA 字符，共 25 行
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 25

#define VGA_CHAR_SIZE 2

// 一个 VGA 字符占 2 字节
#define VGA_ROW_SIZE (VGA_TEXT_WIDTH * VGA_CHAR_SIZE)
// 80x25 共 80*2*25 = 4000 字节
#define VGA_SCR_SIZE (VGA_ROW_SIZE * VGA_TEXT_HEIGHT)

// VGA 显示内存起始地址
#define VGA_TEXT_MEM_BASE 0xb8000
// VGA 显示内存长度
#define VGA_TEXT_MEM_SIZE 0x8000
// VGA 显示内存结束地址，不含 0xc0000
#define VGA_TEXT_MEM_END (VGA_TEXT_MEM_BASE + VGA_TEXT_MEM_SIZE)

// CRTC 寄存器
#define CRT_ADDR_REG 0x3d4
#define CRT_DATA_REG 0x3d5

// 屏幕起始位置寄存器
#define CRT_START_ADDR_H 0xc
#define CRT_START_ADDR_L 0xd
// 光标位置寄存器
#define CRT_CURSOR_H 0xe
#define CRT_CURSOR_L 0xf

// VGA 16 色
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BULE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

#define VGA_ERASE_CHAR 0x0720

typedef unsigned short vga_attributes;

u32 vga_get_screen_start_addr();
u32 vga_set_screen_start_addr(u32 scr_addr);
u32 vga_get_cursor_addr();
u32 vga_set_cursor_addr(u32 cursor_addr);

void vga_text_clear_screen(u32 scr_addr);
void vga_text_clear_memory();
u32 vga_text_scroll_up();

vga_attributes vga_text_set_theme(vga_attributes fg, vga_attributes bg);
u32 vga_text_write_char(u32 addr, vga_attributes attr, char ch);
u32 vga_text_write_str(u32 addr, vga_attributes attr, const char *str,
                       u32 count);

#endif // !OAK_VGA_H
