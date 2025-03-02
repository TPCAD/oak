#include <oak/assert.h>
#include <oak/bitmap.h>
#include <oak/debug.h>
#include <oak/fs.h>
#include <oak/memory.h>
#include <oak/multiboot2.h>
#include <oak/oak.h>
#include <oak/printk.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/task.h>
#include <oak/types.h>

#define ZONE_VALID 1    // valid area of ards
#define ZONE_RESERVED 2 // invalid area of ards

#define IDX(addr) ((u32)addr >> 12)            // page index
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff) // page directory index
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff) // page table index
#define PAGE(idx) ((u32)idx << 12)
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

#define KERNEL_MAP_BITS 0x6000 // array address of kernel virtual memory

#define PDE_MASK 0xffc00000

static u32 KERNEL_PAGE_TABLE[] = {0x2000, 0x3000, 0x4000, 0x5000};

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
    u32 count = 0;

    if (magic == OAK_MAGIC) {
        count = *(u32 *)addr;
        ards_t *ptr = (ards_t *)(addr + 4); // points to ards_buffer

        // find out the largest memory block
        for (size_t i = 0; i < count; i++, ptr++) {
            if (ptr->type == ZONE_VALID && ptr->size > memory_size) {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    } else if (magic == MULTIBOOT2_MAGIC) {
        u32 size = *(unsigned int *)addr;
        multi_tag_t *tag = (multi_tag_t *)(addr + 8);

        DEBUGK("Announced mbi size 0x%x\n", size);

        while (tag->type != MULTIBOOT_TAG_TYPE_END) {
            if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
                break;
            }
            tag = (multi_tag_t *)((u32)tag + ((tag->size + 7) & ~7));
        }

        multi_tag_mmap_t *mtag = (multi_tag_mmap_t *)tag;
        multi_mmap_entry_t *entry = mtag->entries;
        while ((u32)entry < (u32)tag + tag->size) {
            DEBUGK("Memory base 0x%p size 0x%p type %d\n", (u32)entry->addr,
                   (u32)entry->len, (u32)entry->type);
            count++;
            if (entry->type == ZONE_VALID && entry->len > memory_size) {
                memory_base = (u32)entry->addr;
                memory_size = (u32)entry->len;
            }
            entry = (multi_mmap_entry_t *)((u32)entry + mtag->entry_size);
        }
    } else {
        panic("Memory init magic unknown 0x%p\n", magic);
    }

    assert(memory_base == MEMORY_BASE);
    assert((memory_size & 0xfff) == 0); // 要求内存大小按页对齐

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

/*
 *  @brief  内核内存管理初始化
 *
 *  使用数组 u8 memory_map[memory_map_pages * PAGE_SIZE]
 *  表示内核物理内存的分配情况
 *  @a memory_map_pages  用于管理内存的内存页数，一字节代表一页内存
 *  @a memory_map  用于管理内存的数组，置于内存起始位置
 *
 *  使用位图 @a kernel_map  管理内核虚拟内存
 *  @a length  字节为单位的位图长度
 *  */
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
            u32 page = PAGE(i);
            DEBUGK("Allocate page 0x%p\n", page);
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

/* Get the value of cr2
 */
u32 get_cr2() {
    // return value in eax
    asm volatile("movl %cr2, %eax\n");
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

/*
 *  @brief  内核内存映射并开启分页
 *
 *  页表是一个大小为 1024 的数组。页表元素称为页表项，一个页表项大小为 4 字节
 *  因此一个页表的大小为 4K 字节，可以表示 1K 页。
 *
 *  系统采用二级页表。顶级页表称为页目录，一般只有一个页目录。页目录项存储页表
 *  的地址。
 *
 * Place page directory which is 4K bytes in size in 0x1000.Page directory has
 * 3 page table in index 0, 1 and 1023.The top 2 map the virtual 8M to physical
 * 8M(ignore first 4K). The last one in index 1023 points to the directory
 * itself therefore we can modify the page directory and page table through
 * this entry.
 */
void mapping_init() {
    // 初始化页目录
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);

    idx_t index = 0;
    for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++) {
        // 初始化页表
        page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);

        // 初始化页目录项
        page_entry_t *dentry = &pde[didx];
        entry_init(dentry, IDX((u32)pte));
        dentry->user = 0;

        // 初始化页表项
        for (size_t tidx = 0; tidx < 1024; tidx++, index++) {
            // 第 0 个页表不作映射
            if (index == 0) {
                continue;
            }
            page_entry_t *tentry = &pte[tidx];
            entry_init(tentry, index);
            tentry->user = 0;
            memory_map[index] = 1;
        }
    }

    // 最后一个页目录项指向页目录本身
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(KERNEL_PAGE_DIR));

    // enable paging
    set_cr3((u32)pde);
    enable_page();
}

/*
 *  @brief  获取页目录地址
 *  @return  指向页目录的**虚拟地址**
 */
static page_entry_t *get_pde() { return (page_entry_t *)(0xfffff000); }

/*
 *  @breif  获取页表地址
 *  @param  vaddr  虚拟地址
 *  @param  create
 *  @return  指向对应页表的虚拟地址
 **/
static page_entry_t *get_pte(u32 vaddr, bool create) {
    // return (page_entry_t *)(0xffc00000 | (DIDX(vaddr) << 12));

    page_entry_t *pde = get_pde();
    u32 idx = DIDX(vaddr);
    page_entry_t *entry = &pde[idx];

    assert(create || (!create && entry->present));

    page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

    BMB;
    if (!entry->present) {
        DEBUGK("Get and create page table entry for 0x%p\n", vaddr);
        u32 page = alloc_page();
        entry_init(entry, IDX(page));
        memset(table, 0, PAGE_SIZE);
    }
    BMB;

    return table;
}

/*
 *  @breif  获取页表项地址
 *  @param  vaddr  虚拟地址
 *  @param  create
 *  @return  指向对应页表项的虚拟地址
 **/
page_entry_t *get_entry(u32 vaddr, bool create) {
    page_entry_t *pte = get_pte(vaddr, create);
    return &pte[TIDX(vaddr)];
}

/*  @breif  Flush table
 *  @param  vaddr  Virtual address of page table
 */
void flush_tlb(u32 vaddr) {
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

/*
 *  @brief  分配连续虚拟页
 *  @param  map  位图
 *  @param  count  需要分配的页数
 *  @return  分配到的第一个页的地址
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

/*
 *  @breif  释放分配的虚拟页
 *  @param  map  位图
 *  @param  addr  需要释放的页的地址
 *  @param  count  需要释放的页数
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

/*
 *  @brief  分配连续虚拟页
 *  @param  count  需要分配的页数
 *  @return  分配到的第一个页的地址
 *
 *  对 scan_page 的包装
 */
u32 alloc_kpage(u32 count) {
    assert(count > 0);
    u32 vaddr = scan_page(&kernel_map, count);
    DEBUGK("allocate kernel page 0x%p count %d\n", vaddr, count);
    return vaddr;
}

/*
 *  @breif  释放分配的虚拟页
 *  @param  addr  需要释放的页的地址
 *  @param  count  需要释放的页数
 *
 *  对 reset_page 的包装
 */
void free_kpage(u32 vaddr, u32 count) {
    ASSERT_PAGE(vaddr);
    assert(count > 0);
    reset_page(&kernel_map, vaddr, count);
    DEBUGK("free kernel page 0x%p count %d\n", vaddr, count);
}

/**
 *  @brief  绑定虚拟页与物理页
 *  @param  vaddr  虚拟地址
 */
void link_page(u32 vaddr) {
    ASSERT_PAGE(vaddr);

    page_entry_t *entry = get_entry(vaddr, true);

    task_t *task = running_task();
    bitmap_t *map = task->vmap;
    u32 index = IDX(vaddr);

    // if page exist
    if (entry->present) {
        return;
    }

    u32 paddr = alloc_page();
    entry_init(entry, IDX(paddr));
    flush_tlb(vaddr);

    DEBUGK("link from 0x%p to 0x%p\n", vaddr, paddr);
}

/**
 *  @brief  解绑虚拟页与物理页
 *  @param  vaddr  虚拟地址
 */
void unlink_page(u32 vaddr) {
    ASSERT_PAGE(vaddr);

    page_entry_t *pde = get_pde();
    page_entry_t *entry = &pde[DIDX(vaddr)];
    if (!entry->present) {
        return;
    }

    entry = get_entry(vaddr, false);

    // if page not exist
    if (!entry->present) {
        return;
    }

    entry->present = false;

    u32 paddr = PAGE(entry->index);

    DEBUGK("unlink page from 0x%p to 0x%p\n", vaddr, paddr);

    free_page(paddr);
    flush_tlb(vaddr);
}

static u32 copy_page(void *page) {
    // 分配一页物理页
    u32 paddr = alloc_page();
    u32 vaddr = 0;
    // 利用第 0 页进行复制
    page_entry_t *entry = get_pte(vaddr, false);
    // 将分配的物理页绑定到页表项
    entry_init(entry, IDX(paddr));
    flush_tlb(vaddr);
    // 拷贝旧的页到新的物理页，通过虚拟地址找到分配的物理页
    memcpy((void *)vaddr, (void *)page, PAGE_SIZE);

    entry->present = false;
    flush_tlb(vaddr);
    return paddr;
}

page_entry_t *copy_pde() {
    task_t *task = running_task();
    page_entry_t *pde = (page_entry_t *)alloc_kpage(1);
    memcpy(pde, (void *)task->pde, PAGE_SIZE);

    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(pde));

    page_entry_t *dentry;

    for (size_t didx = (sizeof((KERNEL_PAGE_TABLE)) / 4); didx < 1023; didx++) {
        dentry = &pde[didx];
        if (!dentry->present) {
            continue;
        }

        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++) {
            entry = &pte[tidx];
            if (!entry->present) {
                continue;
            }

            assert(memory_map[entry->index] > 0);

            if (!entry->shared) {
                entry->write = false;
            }

            memory_map[entry->index]++;

            assert(memory_map[entry->index] < 255);
        }

        u32 paddr = copy_page(pte);
        dentry->index = IDX(paddr);
    }
    set_cr3(task->pde);

    return pde;
}

void free_pde() {
    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);

    page_entry_t *pde = get_pde();

    for (size_t didx = (sizeof((KERNEL_PAGE_TABLE)) / 4); didx < 1023; didx++) {
        page_entry_t *dentry = &pde[didx];
        if (!dentry->present) {
            continue;
        }

        page_entry_t *pte = (page_entry_t *)(PDE_MASK | (didx << 12));

        for (size_t tidx = 0; tidx < 1024; tidx++) {
            page_entry_t *entry = &pte[tidx];
            if (!entry->present) {
                continue;
            }

            assert(memory_map[entry->index] > 0);
            free_page(PAGE(entry->index));
        }

        free_page(PAGE(dentry->index));
        DEBUGK("free pages %d\n", free_pages);
    }
}

int sys_brk(void *addr) {
    DEBUGK("task brk 0x%p\n", addr);
    u32 brk = (u32)addr;
    ASSERT_PAGE(brk);

    task_t *task = running_task();
    assert(task->uid != KERNEL_USER);

    assert(task->end <= brk && brk <= USER_MMAP_ADDR);

    u32 old_brk = task->brk;

    if (old_brk > brk) {
        for (u32 page = brk; page < old_brk; page += PAGE_SIZE) {
            unlink_page(page);
        }
    } else if (IDX((brk - old_brk)) > free_pages) {
        return -1;
    }
    task->brk = brk;
    return 0;
}

void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd,
               off_t offset) {
    ASSERT_PAGE((u32)addr);

    u32 count = div_round_up(length, PAGE_SIZE);
    u32 vaddr = (u32)addr;

    task_t *task = running_task();
    if (!vaddr) {
        vaddr = scan_page(task->vmap, count);
    }

    assert(vaddr >= USER_MMAP_ADDR && vaddr < USER_STACK_BOTTOM);

    for (size_t i = 0; i < count; i++) {
        u32 page = vaddr + PAGE_SIZE * i;
        link_page(page);
        bitmap_set(task->vmap, IDX(page), true);

        page_entry_t *entry = get_entry(page, false);
        entry->user = true;
        entry->write = false;
        entry->readonly = true;
        if (prot & PROT_WRITE) {
            entry->readonly = false;
            entry->write = true;
        }
        if (flags & MAP_SHARED) {
            entry->shared = true;
        }
        if (flags & MAP_PRIVATE) {
            entry->private = true;
        }
        flush_tlb(page);
    }

    if (fd != EOF) {
        lseek(fd, offset, SEEK_SET);
        read(fd, (char *)vaddr, length);
    }

    return (void *)vaddr;
}

int sys_munmap(void *addr, size_t length) {
    task_t *task = running_task();
    u32 vaddr = (u32)addr;
    assert(vaddr >= USER_MMAP_ADDR && vaddr < USER_STACK_BOTTOM);

    ASSERT_PAGE(vaddr);
    u32 count = div_round_up(length, PAGE_SIZE);

    for (size_t i = 0; i < count; i++) {
        u32 page = vaddr + PAGE_SIZE * i;
        unlink_page(page);
        assert(bitmap_is_set(task->vmap, IDX(page)));
        bitmap_set(task->vmap, IDX(page), false);
    }

    return 0;
}

typedef struct page_error_code_t {
    u8 present : 1;
    u8 write : 1;
    u8 user : 1;
    u8 reserved0 : 1;
    u8 fetch : 1;
    u8 protection : 1;
    u8 shadow : 1;
    u16 reserved1 : 8;
    u8 sgx : 1;
    u16 reserved2;
} _packed page_error_code_t;

void page_fault(u32 vector, u32 edi, u32 esi, u32 ebp, u32 esp, u32 ebx,
                u32 edx, u32 ecx, u32 eax, u32 gs, u32 fs, u32 es, u32 ds,
                u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags) {
    assert(vector == 0xe);
    u32 vaddr = get_cr2();
    DEBUGK("fault address 0x%p\n", vaddr);

    page_error_code_t *code = (page_error_code_t *)&error;
    task_t *task = running_task();

    // assert(KERNEL_MEMORY_SIZE <= vaddr && vaddr < USER_STACK_TOP);
    if (vaddr < USER_EXEC_ADDR || vaddr >= USER_STACK_TOP) {
        assert(task->uid);
        printk("Segmentation Fault!!!\n");
        task_exit(-1);
    }

    if (code->present) {
        assert(code->write);

        page_entry_t *entry = get_entry(vaddr, false);

        assert(entry->present);
        assert(!entry->shared);
        assert(!entry->readonly);

        assert(memory_map[entry->index] > 0);
        if (memory_map[entry->index] == 1) {
            entry->write = true;
            DEBUGK("WRITE page for 0x%p\n", vaddr);
        } else {
            void *page = (void *)PAGE(IDX(vaddr));
            u32 paddr = copy_page(page);
            memory_map[entry->index]--;
            entry_init(entry, IDX(paddr));
            flush_tlb(vaddr);
            DEBUGK("COPY page for 0x%p\n", vaddr);
        }
        return;
    }

    if (!code->present && (vaddr < task->brk || vaddr >= USER_STACK_BOTTOM)) {
        u32 page = PAGE(IDX(vaddr));
        link_page(page);
        return;
    }
    DEBUGK("task 0x%p name %s brk 0x%p page fault\n", task, task->name,
           task->brk);
    panic("page fault");
}
