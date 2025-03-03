#include "x86/vga.h"
#include <oak/io.h>
#include <oak/tty.h>
char msg[] = "Hello Oak!";
char buf[1024];

void kernel_init() { tty_init(); }
