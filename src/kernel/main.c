#include "oak/printk.h"
#include <oak/console.h>
#include <oak/io.h>
#include <oak/oak.h>
#include <oak/printk.h>
#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/types.h>

void kernel_init() {
    console_init();
    printk("%s", "hello");
    return;
}
