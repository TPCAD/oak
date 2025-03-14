#include <oak/debug/kassert.h>
#include <oak/kprintf.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/tty.h>

void kassert_failure(char *exp, char *file, char *base, int line) {
    tty_write_str("\n-> assert(", 11);
    tty_write_str(exp, strlen(exp));
    tty_write_str(") failed!", 9);

    tty_write_str("\n-> file: ", 10);
    tty_write_str(file, strlen(file));

    tty_write_str("\n-> base: ", 10);
    tty_write_str(base, strlen(base));

    char line_str[8];
    itoa(line, line_str);
    tty_write_str("\n-> line: ", 10);
    tty_write_str(line_str, strlen(line_str));
    tty_write_str("\n", 1);

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

    tty_write_str("[kernel] Panic\n", 15);
    tty_write_str(buf, i);

    while (true) {
    }

    asm volatile("ud2");
}
