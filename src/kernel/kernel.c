#include "oak/clock.h"
#include <oak/debug/kdebug.h>
#include <oak/kprintf.h>
#include <oak/mm/memory.h>
#include <oak/mm/paging.h>
#include <oak/mm/pmm.h>
#include <oak/types.h>

extern void tty_init();
extern void pmm_init(u32 mem_upper_lim);
extern void pic_init();
extern void clock_init();

extern mem_info_t mem_info;

void kernel_init() {
    tty_init();

    pmm_init(MEMORY_BASE + (mem_info).max_zone_size);

    for (int i = 0; i < mem_info.ards_count; i++) {
        ards_t *ards = mem_info.ards_arr + i;
        if (ards->type == 1) {
            kprintf("base: %p, size: %p, type: %d\n", (u32)ards->base,
                    (u32)ards->size, (u32)ards->type);
            pmm_mark_chunk_free(IDX(ards->base), ards->size / PAGE_SIZE);
        }
    }

    paging_init();
    pic_init();
    clock_init();
}

void kernel_main() {
    kprintf("Hello Oak!\n");
    // BMB;
    asm volatile("sti\n");
    while (true) {
    }
    // asm volatile("int $0x80\n");
    // int *a = (int *)0x1000000;
    // kprintf("%d\n", *a);
    // BMB;

    // u32 count = 0;
    // while (true) {
    //     KDEBUG("looping in kernel %d...\n", count++);
    //     u32 delay = 100000;
    //     while (delay--) {
    //     }
    // }
    return;
}
