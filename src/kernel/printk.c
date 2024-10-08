#include <oak/console.h>
#include <oak/printk.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/types.h>

extern int32 console_write(void *dev, char *buf, u32 count);

static char buf[1024];

int printk(const char *fmt, ...) {
    va_list args;

    int i;
    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    console_write(NULL, buf, i);
    return i;
}
