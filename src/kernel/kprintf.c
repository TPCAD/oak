#include <oak/kprintf.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/tty.h>

static char buf[1024];

int kprintf(const char *fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);

    int i = vsprintf(buf, fmt, vlist);
    va_end(vlist);

    tty_write_str(buf, i);
    return i;
}
