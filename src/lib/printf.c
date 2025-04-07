#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/syscall.h>

static char buf[1024];

int printf(const char *fmt, ...) {
    va_list vlist = NULL;
    va_start(vlist, fmt);
    int i = vsprintf(buf, fmt, vlist);
    va_end(vlist);
    write(stdout, buf, i);
    return i;
}
