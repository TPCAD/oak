#include <oak/kprintf.h>
#include <oak/stdio.h>
#include <oak/tty.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() {
    tty_init();
    kprintf("%0#10xg\n", 1000);
    kprintf("%#10xg\n", 1000);
    kprintf("%#10.5xg\n", 1000);
    kprintf("%0#10.5xg\n", 1000);
    kprintf("%-#10.5xg\n", 1000);
    kprintf("%-#10xg\n", 1000);
    kprintf("%0-#10xg\n", 1000);
    kprintf("%0-#xg\n", 1000);
    kprintf("%#20.xg\n", 0);

    kprintf("%20.5s|\n", msg);
    kprintf("%20.s|\n", msg);
}
