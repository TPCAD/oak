#include "oak/assert.h"
#include "oak/debug.h"
#include "oak/oak.h"
#include "oak/stdlib.h"
#include "oak/types.h"
#include <oak/memory.h>
#include <oak/string.h>

#define ZONE_VALID 1    // valid area of ards
#define ZONE_RESERVED 2 // invalid area of ards

#define IDX(addr) ((u32)addr >> 12)
#define PAGE(idx) ((u32)idx << 12)
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

typedef struct ards_t {
    u64 base; // base address of memory
    u64 size; // size of memory
    u32 type; // type of memory
} _packed ards_t;

static u32 memory_base = 0;
static u32 memory_size = 0;
static u32 total_pages = 0;
static u32 free_pages = 0;

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

static u32 start_page = 0;   // start location of available physical memory
static u8 *memory_map;       // physical memory array
static u32 memory_map_pages; // pages

void memory_map_init() {
    // pages amount to manage physical memory
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);
    DEBUGK("Memory map pages count: %d\n", memory_map_pages);

    // use start `memory_map_pages` pages to manage physical memory
    memory_map = (u8 *)memory_base;
    memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);

    free_pages -= memory_map_pages;

    start_page = IDX(memory_base) + memory_map_pages;

    for (size_t i = 0; i < start_page; i++) {
        memory_map[i] = 1;
    }

    DEBUGK("Total pages %d, free pages %d\n", total_pages, free_pages);
}

static u32 alloc_page() {
    for (size_t i = start_page; i < total_pages; i++) {
        if (!memory_map[i]) {
            memory_map[i] = 1;
            free_pages--;
            u32 page = ((u32)i) << 12;
            DEBUGK("Allocate page 0x%p\n", page);
            return page;
        }
    }
    panic("Out of memory");
    return 0; // not necessary
}

static void free_page(u32 addr) {
    // must be start location of a page
    ASSERT_PAGE(addr);

    u32 idx = IDX(addr);

    assert(idx >= start_page && idx < total_pages);

    // reference must not be 0
    assert(memory_map[idx] >= 1);

    memory_map[idx]--;

    if (!memory_map[idx]) {
        free_pages++;
    }

    assert(free_pages > 0 && free_pages < total_pages);
    DEBUGK("Free page 0x%p\n", addr);
}

u32 get_cr3() {
    // return value in eax
    asm volatile("movl %cr3, %eax\n");
}

// set cr3, pde is the address of page index
void set_cr3(u32 pde) {
    ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

// set the highest bit of cr0 to 1 to enable page
static void enable_page() {
    // 0b1000_0000_0000_0000_0000_0000_0000_0000
    // 0x80000000
    asm volatile("movl %cr0, %eax\n"
                 "orl $0x80000000, %eax\n"
                 "movl %eax, %cr0\n");
}

// init page table entry or page directory entry
static void entry_init(page_entry_t *entry, u32 index) {
    *(u32 *)entry = 0;
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

#define KERNEL_PAGE_DIR 0x200000
#define KERNEL_PAGE_ENTRY 0x201000

void mapping_init() {
    // page directory entry
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    entry_init(&pde[0], IDX(KERNEL_PAGE_ENTRY));

    // page table entry
    page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_ENTRY;
    memset(pte, 0, PAGE_SIZE);

    // map start 8M to physical 8M
    page_entry_t *entry;
    for (size_t tidx = 0; tidx < 1024; tidx++) {
        entry = &pte[tidx];
        entry_init(entry, tidx);
        memory_map[tidx] = 1;
    }

    set_cr3((u32)pde);

    enable_page();
}

void memory_test() {
    u32 pages[10];
    for (size_t i = 0; i < 10; i++) {
        pages[i] = alloc_page();
    }
    DEBUGK("Total pages %d, free pages %d\n", total_pages, free_pages);

    for (size_t i = 0; i < 10; i++) {
        free_page(pages[i]);
    }
    DEBUGK("Total pages %d, free pages %d\n", total_pages, free_pages);
}
