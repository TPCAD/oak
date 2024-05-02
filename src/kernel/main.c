#include <oak/console.h>
#include <oak/io.h>
#include <oak/oak.h>
#include <oak/types.h>

char message[] = "hello world!\n";

void kernel_init() {
    console_init();
    while (true) {
        consoel_write(message, sizeof(message) - 1);
    }
    return;
}
