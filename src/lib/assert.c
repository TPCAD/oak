#include <oak/assert.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/types.h>

static char buf[1024];

static void spin(char *name) {
    printf("spinning in %s ...\n", name);
    while (true)
        ;
}

void assert_failure(char *exp, char *file, char *base, int line) {
    printf("\n--> assert(%s) failed!"
           "\n--> file: %s"
           "\n--> base: %s"
           "\n--> line: %d\n",
           exp, file, base, line);

    spin("assert_failure()");

    // error if execute this
    asm volatile("ud2");
}

void panic(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int i = vsprintf(buf, fmt, args);
    va_end(args);

    printf("panic!\n--> %s\n", buf);
    spin("panic()");

    // error if execute this
    asm volatile("ud2");
}
