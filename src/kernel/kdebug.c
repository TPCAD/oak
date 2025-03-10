#include <oak/kprintf.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>

static char buf[1024];

void kdebug(char *file, int line, const char *fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);
    vsprintf(buf, fmt, vlist);
    kprintf("[%s] [%d] %s", file, line, buf);
}
