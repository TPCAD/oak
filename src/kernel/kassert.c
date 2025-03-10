#include <oak/kassert.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/tty.h>

static void kspin(char *func_name) {
    tty_write_str("spinning in ", 12);
    tty_write_str(func_name, strlen(func_name));
    tty_write_str(" ...\n", 5);
    while (true) {
    }
}

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

    kspin("assert_failure()");

    asm volatile("ud2");
}
