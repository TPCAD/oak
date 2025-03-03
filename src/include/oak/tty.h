#ifndef OAK_CONSOLE_H
#define OAK_CONSOLE_H

#include <x86/vga.h>

#define ASCII_NUL 0x00
#define ASCII_ENQ 0x05
#define ASCII_BEL 0x07 // \a
#define ASCII_BS 0x08  // \b
#define ASCII_HT 0x09  // \t
#define ASCII_LF 0x0a  // \n
#define ASCII_VT 0x0b  // \v
#define ASCII_FF 0x0c  // \f
#define ASCII_CR 0x0d  // \r
#define ASCII_DEL 0x7f

void tty_set_theme(vga_attributes fg, vga_attributes bg);
// void tty_write_char(char chr);
void tty_write_str(char *buf, u32 count);
void tty_scroll_up();
void tty_clear();

void tty_init();

#endif // !OAK_CONSOLE_H
