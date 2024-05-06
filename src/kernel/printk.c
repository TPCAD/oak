#include "oak/printk.h"
#include <oak/console.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>

static char buf[1024];

int printk(const char *fmt, ...) {
    va_list args;

    int i;
    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    consoel_write(buf, i);
    return i;
}
