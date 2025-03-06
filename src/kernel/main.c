#include <oak/stdio.h>
#include <oak/tty.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() {
    tty_init();
    int i = sprintf(buf, "%0#10xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%#10xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%#10.5xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%0#10.5xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%-#10.5xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%-#10xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%0-#10xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%0-#xg\n", 1000);
    tty_write_str(buf, i);
    i = sprintf(buf, "%#20.xg\n", 0);
    tty_write_str(buf, i);

    i = sprintf(buf, "%20.5s|\n", msg);
    tty_write_str(buf, i);
    i = sprintf(buf, "%20.s|\n", msg);
    tty_write_str(buf, i);
}
