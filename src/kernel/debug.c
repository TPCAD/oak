#include "oak/printk.h"
#include <oak/debug.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>

static char buf[1024];

void debugk(char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    printk("[%s] [%d] %s", file, line, buf);
}
