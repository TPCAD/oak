#include <oak/debug/kassert.h>
#include <oak/mm/memory.h>
#include <oak/oak.h>
#include <oak/types.h>

mem_info_t mem_info = {0, NULL, 0, 0};

void parse_ards(u32 magic, u32 ards_count_addr) {
    if (magic == OAK_MAGIC) {
        mem_info.ards_count = *(u32 *)ards_count_addr;
        ards_t *ptr = (ards_t *)(ards_count_addr + 4);
        mem_info.ards_arr = ptr;
        for (size_t i = 0; i < mem_info.ards_count; i++, ptr++) {
            // kprintf("base: %p, size: %p, type: %d\n", (u32)ptr->base,
            //         (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == VALID_MEM_ZONE &&
                ptr->size > mem_info.max_zone_size) {
                mem_info.max_zone_base = ptr->base;
                mem_info.max_zone_size = ptr->size;
            }
        }
    } else {
        kpanic("[mm] Unknown memory magic\n");
    }

    kassert(mem_info.max_zone_base == MEMORY_BASE);
    kassert((mem_info.max_zone_size & 0xfff) == 0);

    if (mem_info.max_zone_size < KERNEL_MEM_END) {
        kpanic("[mm] System memory is too small, at least %dM needed\n",
               KERNEL_MEM_END / MEMORY_BASE);
    }
}
