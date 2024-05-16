#include "oak/assert.h"
#include "oak/debug.h"
#include "oak/oak.h"
#include "oak/types.h"
#include <oak/memory.h>

#define ZONE_VALID 1    // valid area of ards
#define ZONE_RESERVED 2 // invalid area of ards

#define IDX(addr) ((u32)addr >> 12)

typedef struct ards_t {
    u64 base; // base address of memory
    u64 size; // size of memory
    u32 type; // type of memory
} _packed ards_t;

u32 memory_base = 0;
u32 memory_size = 0;
u32 total_pages = 0;
u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

void memory_init(u32 magic, u32 addr) {
    u32 count;
    ards_t *ptr;

    if (magic == OAK_MAGIC) {
        count = *(u32 *)addr;
        ptr = (ards_t *)(addr + 4);
        for (size_t i = 0; i < count; i++, ptr++) {
            DEBUGK("Memory base 0x%p size 0x%p type %d\n", (u32)ptr->base,
                   (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == ZONE_VALID && ptr->size > memory_size) {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    } else {
        panic("Memory init magic unknown 0x%p\n", magic);
    }

    DEBUGK("ARDS count %d\n", count);
    DEBUGK("Memory base 0x%p\n", (u32)memory_base);
    DEBUGK("Memory size 0x%p\n", (u32)memory_size);

    assert(memory_base == MEMORY_BASE);
    assert((memory_size & 0xfff) == 0);

    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);

    DEBUGK("Total pages %d\n", total_pages);
    DEBUGK("Free pages %d\n", free_pages);
}
