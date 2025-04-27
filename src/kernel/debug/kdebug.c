#include "oak/vdevice.h"
#include <oak/kprintf.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>

static char buf[1024];

void kdebug(char *file, int line, const char *fmt, ...) {
    vdevice_t *vdev = vdevice_search(VDEV_SERIAL, 0);
    if (!vdev) {
        vdev = vdevice_search(VDEV_CONSOLE, 0);
    }

    int i = sprintf(buf, "[%s] [%d] ", file, line);
    vdevice_write(vdev->dev, buf, i, 0, 0);

    va_list vlist;
    va_start(vlist, fmt);
    i = vsprintf(buf, fmt, vlist);
    va_end(vlist);
    vdevice_write(vdev->dev, buf, i, 0, 0);
}
