#include <oak/debug/kdebug.h>
#include <oak/kprintf.h>
#include <oak/mm/memory.h>
#include <oak/mm/paging.h>
#include <oak/mm/pmm.h>
#include <oak/types.h>

extern void tty_init();
extern void pmm_init(u32 mem_upper_lim);

extern mem_info_t mem_info;

void kernel_init() {
    tty_init();

    pmm_init(MEMORY_BASE + (mem_info).max_zone_size);

    for (int i = 0; i < mem_info.ards_count; i++) {
        BMB;
        ards_t *ards = mem_info.ards_arr + i;
        if (ards->type == 1) {
            kprintf("base: %p, size: %p, type: %d\n", (u32)ards->base,
                    (u32)ards->size, (u32)ards->type);
            pmm_mark_chunk_free(IDX(ards->base), ards->size / PAGE_SIZE);
        }
    }

    paging_init();
}

void kernel_main() {
    kprintf("Hello Oak!\n");
    BMB;
    asm("int $0x24");
    BMB;
}
