#include "oak/stdlib.h"
#include <oak/kprintf.h>
#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/tty.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() {
    tty_init();

    char tmp[8];
    itoa(1024, tmp);
    kprintf("%s\n", tmp);
    itoa(-1024, tmp);
    kprintf("%s\n", tmp);
}
