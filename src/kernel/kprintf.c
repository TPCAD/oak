#include <oak/kprintf.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/tty.h>
#include <oak/vdevice.h>

static char buf[1024];

int kprintf(const char *fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);

    int i = vsprintf(buf, fmt, vlist);
    va_end(vlist);

    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, buf, i, 0, 0);
    return i;
}
