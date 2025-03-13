#include <oak/gdt.h>
#include <oak/idt.h>
#include <oak/kdebug.h>
#include <oak/kprintf.h>
#include <oak/tty.h>

void kernel_init() {
    gdt_init();
    idt_init();
}

void kernel_main() {
    tty_init();
    kprintf("Hello Oak!\n");
    BMB;
    asm("int $0x24");
    BMB;
}
