#ifndef OAK_MM_MEMORY_H
#define OAK_MM_MEMORY_H

#include <oak/types.h>

#define VALID_MEM_ZONE 1
#define RESERVED_MEM_ZONE 2

#define PAGE_SIZE 0x1000 // 4K

#define MEMORY_BASE 0x100000      // 1 MB
#define KERNEL_MEM_SIZE 0x1000000 // 16 MB

#define KERNEL_PAGE_DIR_ADDR 0x1000 // 内核页目录地址

/**
 *  分页机制下，32 位地址中的高 10 位是页目录索引，中间 10 位是页表索引，低 12
 *  位是页内偏移。
 *
 *  IDX 可以得到地址所在页的索引，这个索引还可以表示在此之前有多少页。
 *
 *  PAGE 则相反，可以通过索引得到对应页的首地址。
 */
#define IDX(addr) ((u32)addr >> 12)
#define TIDX(addr) ((u32)addr >> 12 & 0x3ff)
#define DIDX(addr) ((u32)addr >> 22 & 0x3ff)
#define PIDX(addr) ((u32)addr & 0x00000fffU)

#define PG_ADDR(idx) ((u32)idx << 12)
#define PAGE_ALIGN(addr) ((u32)addr & 0xfffff000U)

#define ASSERT_PAGE(addr) kassert((addr & 0xfff) == 0)

/* 页目录/页表属性 */
#define PG_PRESENT (0x1)
#define PG_WRITE (0x1 << 1)
#define PG_USER (0x1 << 2)
#define PG_WRITE_THROUGH (0x1 << 3)
#define PG_CACHE_DISABLE (0x1 << 4)
#define PG_PDE_4MB (0x1 << 7)

#define PG_ATTR_P (PG_PRESENT)
#define PG_ATTR_PW (PG_PRESENT | PG_WRITE)
#define PG_ATTR_PWU (PG_PRESENT | PG_WRITE | PG_USER)

#define PG_IS_PRESENT(entry) (0x1 & (entry))

/* 创建页目录项/页表项 */
#define PDE(addr, attr) (PAGE_ALIGN(addr) | ((attr) & 0xfff))
#define PTE(addr, attr) (PAGE_ALIGN(addr) | ((attr) & 0xfff))

/* 常用虚拟地址 */
#define PD_BASE_VADDR 0xfffff000U // 页目录的虚拟基地址
#define PT_BASE_VADDR 0xffc00000U // 页表的虚拟基地址

/* 通过页表虚拟基地址和页目录索引构建页表的虚拟地址 */
#define PT_VADDR(pd_idx) (PT_BASE_VADDR | ((u32)pd_idx << 12))

/* 通过页目录索引，页表索引，页内索引构建虚拟地址 */
#define VADDR(pd_idx, pt_idx, pg_idx)                                          \
    ((pd_idx) << 22 | (pt_idx) << 12 | (pg_idx))

typedef u32 page_entry_t;

typedef u32 page_attr_t;

typedef struct ards_t {
    u64 base;
    u64 size;
    u32 type;
} __pack ards_t;

typedef struct mem_info_t {
    u32 ards_count;
    ards_t *ards_arr;
    u32 max_zone_base;
    u32 max_zone_size;
} __pack mem_info_t;

#endif // !OAK_MM_MEMORY_H
