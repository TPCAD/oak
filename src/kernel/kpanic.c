#include <oak/kpanic.h>
#include <oak/kprintf.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/types.h>

static char buf[1024];
void kpanic(const char *fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);
    int i = vsprintf(buf, fmt, vlist);
    va_end(vlist);

    kprintf("kernel panic\n%s \n", buf);

    while (true) {
    }

    asm volatile("ud2");
}
