#include <oak/kassert.h>
#include <oak/kprintf.h>
#include <oak/stdio.h>
#include <oak/tty.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() {
    tty_init();

    kassert(3 > 5);
}
