#include <oak/debug/kassert.h>
#include <oak/kprintf.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/tty.h>
#include <oak/vdevice.h>

void kassert_failure(char *exp, char *file, char *base, int line) {
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, "\n-> assert(", 11, 0,
                  0);
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, exp, strlen(exp), 0,
                  0);

    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, ") failed!", 9, 0, 0);

    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, "\n-> file: ", 10, 0,
                  0);
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, file, strlen(file), 0,
                  0);

    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, "\n-> base: ", 10, 0,
                  0);
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, base, strlen(base), 0,
                  0);

    char line_str[8];
    itoa(line, line_str);
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, "\n-> line: ", 10, 0,
                  0);
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, line_str,
                  strlen(line_str), 0, 0);
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, "\n", 1, 0, 0);

    while (true) {
    }

    asm volatile("ud2");
}

static char buf[1024];
void kpanic(const char *fmt, ...) {
    va_list vlist;
    va_start(vlist, fmt);
    int i = vsprintf(buf, fmt, vlist);
    va_end(vlist);

    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, "[kernel] Panic\n",
                  15, 0, 0);
    vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, buf, i, 0, 0);

    while (true) {
    }

    asm volatile("ud2");
}
