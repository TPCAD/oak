#include "oak/mm/memory.h"
#include "oak/types.h"
#include <oak/debug/kdebug.h>
#include <oak/kprintf.h>

extern void tty_init();
extern void pmm_init(u32 mem_upper_lim);

extern mem_info_t mem_info;

void kernel_init() { tty_init(); }

void kernel_main() {
    kprintf("Hello Oak!\n");
    BMB;
    asm("int $0x24");
    BMB;
}
