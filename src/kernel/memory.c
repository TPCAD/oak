#include <oak/assert.h>
#include <oak/bitmap.h>
#include <oak/debug.h>
#include <oak/memory.h>
#include <oak/oak.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/types.h>

#define ZONE_VALID 1    // valid area of ards
#define ZONE_RESERVED 2 // invalid area of ards

#define IDX(addr) ((u32)addr >> 12)            // page index
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff) // page directory index
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff) // page table index
#define PAGE(idx) ((u32)idx << 12)
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

#define KERNEL_PAGE_DIR 0x1000

static u32 KERNEL_PAGE_TABLE[] = {0x2000, 0x3000};

#define KERNEL_MEMORY_SIZE (0x100000 * sizeof(KERNEL_PAGE_TABLE))

#define KERNEL_MAP_BITS 0x4000

/* A struct to hold ards
 *
 * @FIELD base Base address of memory block
 * @FIELD size Size of memory block
 * @FIELD type Type of memory block
 */
typedef struct ards_t {
    u64 base;
    u64 size;
    u32 type;
} _packed ards_t;

static u32 memory_base = 0;
static u32 memory_size = 0;
static u32 total_pages = 0;
static u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

/* Check memory status
 *
 * This function check the result of `detecting_memory` in loader.asm and store
 * the value about memory status in four static variable
 *
 * @param magic Magic number to compatible with grub
 * @param addr Address of ards_count
 */
void memory_init(u32 magic, u32 addr) {
    u32 count;
    ards_t *ptr;

    if (magic == OAK_MAGIC) {
        count = *(u32 *)addr;
        ptr = (ards_t *)(addr + 4); // points to ards_buffer

        // find out the largest memory block
        for (size_t i = 0; i < count; i++, ptr++) {
            if (ptr->type == ZONE_VALID && ptr->size > memory_size) {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    } else {
        panic("Memory init magic unknown 0x%p\n", magic);
    }

    assert(memory_base == MEMORY_BASE);
    assert((memory_size & 0xfff) == 0);

    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);

    if (memory_size < KERNEL_MEMORY_SIZE) {
        panic("System memory is too small, at least %dM needed\n",
              KERNEL_MEMORY_SIZE / MEMORY_BASE);
    }
}

static u32 start_page = 0;   // start location of available physical memory
static u8 *memory_map;       // physical memory array
static u32 memory_map_pages; // pages amount for managing

bitmap_t kernel_map;

void memory_map_init() {
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);

    // place the array in memory_base(0x100000)
    memory_map = (u8 *)memory_base;
    // size of the array
    memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);

    free_pages -= memory_map_pages;

    start_page = IDX(memory_base) + memory_map_pages;

    for (size_t i = 0; i < start_page; i++) {
        memory_map[i] = 1;
    }

    // DEBUGK("Total pages %d, free pages %d\n", total_pages, free_pages);

    // manage kernel virtual memory with bitmap
    u32 length = IDX(KERNEL_MEMORY_SIZE) / 8;
    bitmap_init(&kernel_map, (u8 *)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    // pages for memory_map
    bitmap_scan(&kernel_map, memory_map_pages);
}

/* Allocate one physical page
 *
 * @return The page address
 */
static u32 alloc_page() {
    for (size_t i = start_page; i < total_pages; i++) {
        if (!memory_map[i]) {
            memory_map[i] = 1;
            free_pages--;
            u32 page = ((u32)i) << 12;
            // DEBUGK("Allocate page 0x%p\n", page);
            return page;
        }
    }
    panic("Out of memory");
    return 0; // not necessary
}

/* Free one physical page
 *
 * @param addr Address of page to free
 */
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
    // DEBUGK("Free page 0x%p\n", addr);
}

/* Get the value of cr3
 */
u32 get_cr3() {
    // return value in eax
    asm volatile("movl %cr3, %eax\n");
}

/* Set the value of cr3
 *
 * @param pde The address of page directory
 */
void set_cr3(u32 pde) {
    ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

/* Enable paging
 *
 * Set the highest bit of cr0 to 1 to enable paging
 */
static _inline void enable_page() {
    // 0b1000_0000_0000_0000_0000_0000_0000_0000
    // 0x80000000
    asm volatile("movl %cr0, %eax\n"
                 "orl $0x80000000, %eax\n"
                 "movl %eax, %cr0\n");
}

/* Initialize page table entry of page directory entry
 *
 * @param entry Address of entry
 * @param index Address of page table or page directory
 */
static void entry_init(page_entry_t *entry, u32 index) {
    *(u32 *)entry = 0;
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

/* Mapping kernel memory and enable paging
 *
 * Place page directory which is 4K bytes in size in 0x1000.Page directory has
 * 3 page table in index 0, 1 and 1023.The top 2 map the virtual 8M to physical
 * 8M(ignore first 4K). The last one in index 1023 points to the directory
 * itself therefore we can modify the page directory and page table through
 * this entry.
 */
void mapping_init() {
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    idx_t index = 0;
    for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++) {
        page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);

        page_entry_t *dentry = &pde[didx];
        entry_init(dentry, IDX((u32)pte));

        // mapping
        for (size_t tidx = 0; tidx < 1024; tidx++, index++) {
            if (index == 0) {
                continue;
            }
            page_entry_t *tentry = &pte[tidx];
            entry_init(tentry, index);
            memory_map[index] = 1;
        }
    }

    // last page table index points to page directory itself
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(KERNEL_PAGE_DIR));

    // enable paging
    set_cr3((u32)pde);
    enable_page();
}

/* Get page directory
 *
 * @return Address of page directory
 */
static page_entry_t *get_pde() { return (page_entry_t *)(0xfffff000); }

/* Get page table entry
 *
 * @param vaddr Address of page?
 *
 * @return Address of page table
 */
static page_entry_t *get_pte(u32 vaddr) {
    return (page_entry_t *)(0xffc00000 | (DIDX(vaddr) << 12));
}

/* Flush table
 *
 * @param vaddr Virtual address of page table
 */
static void flush_tlb(u32 vaddr) {
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

/* Get continuous page
 *
 * @param map Bitmap
 * @param count Amount of pages to allcate
 *
 * @return Address of page allocated
 */
static u32 scan_page(bitmap_t *map, u32 count) {
    assert(count > 0);
    int index = bitmap_scan(map, count);

    if (index == EOF) {
        panic("Not enough pages!\n");
    }

    u32 addr = PAGE(index);
    DEBUGK("Scan page 0x%p count %d\n", addr, count);
    return addr;
}

/* Reset page
 *
 * @param map Bitmap
 * @param addr Address of page
 * @param count Amount of page
 */
static void reset_page(bitmap_t *map, u32 addr, u32 count) {
    ASSERT_PAGE(addr);
    assert(count > 0);

    u32 index = IDX(addr);

    for (size_t i = 0; i < count; i++) {
        assert(bitmap_is_set(map, index + i));
        bitmap_set(map, index + i, false);
    }
}

/* Allocate continuous kernel page
 *
 * @param count Amount of pages
 *
 * @return Address of start page
 */
u32 alloc_kpage(u32 count) {
    assert(count > 0);
    u32 vaddr = scan_page(&kernel_map, count);
    DEBUGK("allocate kernel page 0x%p count %d\n", vaddr, count);
    return vaddr;
}

/* Free continuous kernel page
 *
 * @param vaddr Address of page to free
 * @param count Amount of pages
 *
 * @return Address of start page
 */
void free_kpage(u32 vaddr, u32 count) {
    ASSERT_PAGE(vaddr);
    assert(count > 0);
    reset_page(&kernel_map, vaddr, count);
    DEBUGK("free kernel page 0x%p count %d\n", vaddr, count);
}

void memory_test() {
    u32 *pages = (u32 *)(0x200000);
    u32 count = 0x6ff;
    for (size_t i = 0; i < count; i++) {
        pages[i] = alloc_kpage(1);
        DEBUGK("0x%x\n", i);
    }

    for (size_t i = 0; i < count; i++) {
        free_kpage(pages[i], 1);
    }
}
