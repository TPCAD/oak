#include <oak/gdt.h>
#include <oak/kprintf.h>
#include <oak/tty.h>

void kernel_init() { gdt_init(); }

void kernel_main() {
    tty_init();
    kprintf("Hello Oak!\n");
}
