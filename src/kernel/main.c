#include <oak/io.h>
#include <x86/vga.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() {
    char *video = (char *)0xb8000;
    for (int i = 0; i < sizeof(msg); i++) {
        video[i * 2] = msg[i];
    }
    vga_text_write_char(0xb8001, 0x0700, 'M');
    vga_text_write_str(0xb8001, 0x0700, "OAK");
    vga_text_scroll_up();
}
