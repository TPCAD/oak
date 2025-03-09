#include "oak/stdlib.h"
#include <oak/kprintf.h>
#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/tty.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() {
    tty_init();

    kprintf("%s\n", msg);
    reverse(msg);
    kprintf("%s\n", msg);
}
