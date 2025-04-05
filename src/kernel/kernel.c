#include <oak/cpu.h>
#include <oak/debug/kdebug.h>
#include <oak/kprintf.h>
#include <oak/mm/memory.h>
#include <oak/mm/pmm.h>
#include <oak/types.h>

extern void tty_init();
extern void pmm_init(u32 mem_upper_lim);
extern void pic_init();
extern void clock_init();
extern void task_init();
extern void paging_init();
extern void syscall_init();

extern mem_info_t mem_info;

void kernel_init() {
    tty_init();

    pmm_init(MEMORY_BASE + (mem_info).max_zone_size);

    for (int i = 0; i < mem_info.ards_count; i++) {
        ards_t *ards = mem_info.ards_arr + i;
        // kprintf("base: %p, size: %p, type: %d\n", (u32)ards->base,
        //         (u32)ards->size, (u32)ards->type);
        if (ards->type == 1) {
            pmm_mark_chunk_free(IDX(ards->base), ards->size / PAGE_SIZE);
        }
    }

    // 标记前 1M 为已占用
    pmm_mark_chunk_occupied(0, IDX(MEMORY_BASE));
    // 标记物理内存数组所用页为已占用
    // FIX: 所用页数量应通过计算得到
    pmm_mark_chunk_occupied(IDX(MEMORY_BASE), 2);

    paging_init();
    pic_init();
    clock_init();
    task_init();
    syscall_init();
}

extern void vmm_test();
void kernel_main() {
    kprintf("Hello Oak!\n");
    // cpu_set_intr_state(true);
    BMB;
    asm volatile("movl $0, %eax\n"
                 "int $0x80\n");
    kprintf("Hello Oak!\n");
    // vmm_test();
    return;
}
