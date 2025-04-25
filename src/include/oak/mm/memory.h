#ifndef OAK_MM_MEMORY_H
#define OAK_MM_MEMORY_H

#include <oak/types.h>

#define VALID_MEM_ZONE 1
#define RESERVED_MEM_ZONE 2

#define PAGE_SIZE 0x1000 // 4K

#define KERNEL_PAGE_DIR_ADDR 0x1000 // 内核页目录地址

#define MEMORY_BASE 0x100000 // 可用内存起始位置 1MB

#define KERNEL_BUFFER_ADDR 0x800000 // 高速缓冲区起始地址 8MB
#define KERNEL_BUFFER_SIZE 0x400000 // 高速缓冲区大小 4MB

// 虚拟磁盘起始地址 12MB
#define KERNEL_RAMDISK_ADDR (KERNEL_BUFFER_ADDR + KERNEL_BUFFER_SIZE)
// 虚拟磁盘大小地址 4MB
#define KERNEL_RAMDISK_SIZE 0x400000

#define KERNEL_MEM_END 0x1000000 // 内核内存结束地址 16MB

#define USER_EXEC_ADDR KERNEL_MEM_END // 用户程序地址 16MB

#define USER_MMAP_ADDR 0x8000000 // 用户映射内存起始地址 128MB
#define USER_MMAP_SIZE 0x8000000 // 用户映射内存大小 128MB

#define USER_STACK_BOTTOM 0x10000000 // 用户栈底 256MB
#define USER_STACK_SIZE 0x200000     // 2MB
#define USER_STACK_TOP (USER_STACK_BOTTOM - USER_STACK_SIZE)

/*
在 32 位操作系统中，一个页表项的大小为 4 字节。

```language
|31                      12|11  9| 8 |  7  | 6 | 5 |  4  |  3  |  2  |  1  | 0 |
+--------------------------+-----+---+-----+---+---+-----+-----+-----+-----+---+
|         address          | AVL | G | PAT | D | A | PCD | PWT | U/S | R/W | P |
+--------------------------+-----+---+-----+---+---+-----+-----+-----+-----+---+
```

- Present：1 位，是否在内存中
- R/W：1 位，0 表示只读，1 表示可读可写
- U/S：1 位，0 表示仅超级用户可访问，1 表示所有人可访问
- PWT：1 位，0 表示直写模式，1 表示回写模式
- PCD：1 位，0 表示该页会被缓存，1 表示不会
- A：1 位，0 表示被访问过，1 表示没有
- D：1 位，0 表示该页未被写过，1表示该页被写过
- PAT：1 位， PAT 是否支持
- G：1 位，是否为全局页（所有进程都需要用到）
- Address：20 位，页表的物理地址（4K 对齐）
*/

/* 页目录/页表属性 */
#define PG_PRESENT (0x1)
#define PG_WRITE (0x1 << 1)
#define PG_USER (0x1 << 2)
#define PG_WRITE_THROUGH (0x1 << 3)
#define PG_CACHE_DISABLE (0x1 << 4)
#define PG_ACCESSED (0x1 << 5)
#define PG_DIRTY (0x1 << 6)
#define PG_PDE_4MB (0x1 << 7)
#define PG_GLOBAL (0x1 << 8)
/* 9～11 位是还未使用的，这里将第 9 位用作共享位，第 10 位用作私有位，
 * 第 11 位用作只读位。 */
#define PG_SHARED (0x1 << 9)
#define PG_PRIVATE (0x1 << 10)
#define PG_RDONLY (0x1 << 11)

// 判断是否设置页表项属性
#define PG_IS_PRESENT(entry) (PG_PRESENT & (u32)(entry))
#define PG_IS_WRITE(entry) (PG_WRITE & (u32)(entry))
#define PG_IS_USER(entry) (PG_USER & (u32)(entry))
#define PG_IS_WRITE_THROUGH(entry) (PG_WRITE_THROUGH & (u32)(entry))
#define PG_IS_CACHE_DISABLE(entry) (PG_CACHE_DISABLE & (u32)(entry))
#define PG_IS_ACCESSED(entry) (PG_ACCESSED & (u32)(entry))
#define PG_IS_DIRTY(entry) (PG_DIRTY & (u32)(entry))
#define PG_IS_PDE_4MB(entry) (PG_PDE_4MB & (u32)(entry))
#define PG_IS_GLOBAL(entry) (PG_GLOBAL & (u32)(entry))
#define PG_IS_SHARED(entry) (PG_SHARED & (u32)(entry))
#define PG_IS_PRIVATE(entry) (PG_PRIVATE & (u32)(entry))
#define PG_IS_RDONLY(entry) (PG_RDONLY & (u32)(entry))

// 设置页表项属性
#define PG_SET_PRESENT(entry) (PG_PRESENT | (u32)(entry))
#define PG_SET_WRITE(entry) (PG_WRITE | (u32)(entry))
#define PG_SET_USER(entry) (PG_USER | (u32)(entry))
#define PG_SET_WRITE_THROUGH(entry) (PG_WRITE_THROUGH | (u32)(entry))
#define PG_SET_CACHE_DISABLE(entry) (PG_CACHE_DISABLE | (u32)(entry))
#define PG_SET_ACCESSED(entry) (PG_ACCESSED | (u32)(entry))
#define PG_SET_DIRTY(entry) (PG_DIRTY | (u32)(entry))
#define PG_SET_PDE_4MB(entry) (PG_PDE_4MB | (u32)(entry))
#define PG_SET_GLOBAL(entry) (PG_GLOBAL | (u32)(entry))
#define PG_SET_SHARED(entry) (PG_SHARED | (u32)(entry))
#define PG_SET_PRIVATE(entry) (PG_PRIVATE | (u32)(entry))
#define PG_SET_RDONLY(entry) (PG_RDONLY | (u32)(entry))

// 删除页表项属性
#define PG_UNSET_PRESENT(entry) (~PG_PRESENT & (u32)(entry))
#define PG_UNSET_WRITE(entry) (~PG_WRITE & (u32)(entry))
#define PG_UNSET_USER(entry) (~PG_USER & (u32)(entry))
#define PG_UNSET_WRITE_THROUGH(entry) (~PG_WRITE_THROUGH & (u32)(entry))
#define PG_UNSET_CACHE_DISABLE(entry) (~PG_CACHE_DISABLE & (u32)(entry))
#define PG_UNSET_ACCESSED(entry) (~PG_ACCESSED & (u32)(entry))
#define PG_UNSET_DIRTY(entry) (~PG_DIRTY & (u32)(entry))
#define PG_UNSET_PDE_4MB(entry) (~PG_PDE_4MB & (u32)(entry))
#define PG_UNSET_GLOBAL(entry) (~PG_GLOBAL & (u32)(entry))
#define PG_UNSET_SHARED(entry) (~PG_SHARED & (u32)(entry))
#define PG_UNSET_PRIVATE(entry) (~PG_PRIVATE & (u32)(entry))
#define PG_UNSET_RDONLY(entry) (~PG_RDONLY & (u32)(entry))

#define PG_ATTR_P (PG_PRESENT)
#define PG_ATTR_PW (PG_PRESENT | PG_WRITE)
#define PG_ATTR_PWU (PG_PRESENT | PG_WRITE | PG_USER)

#define PG_ATTR_W (PG_WRITE)
#define PG_ATTR_WU (PG_WRITE | PG_USER)

/**
 *  分页机制下，32 位地址中的高 10 位是页目录索引，中间 10 位是页表索引，低 12
 *  位是页内偏移。
 *
 *  IDX 可以得到地址所在页的索引，这个索引还可以表示在此之前有多少页。
 *
 *  PG_ADDR 则相反，可以通过索引得到对应页的首地址。
 */
#define IDX(addr) ((u32)addr >> 12)
#define TIDX(addr) ((u32)addr >> 12 & 0x3ff)
#define DIDX(addr) ((u32)addr >> 22 & 0x3ff)
#define PIDX(addr) ((u32)addr & 0x00000fffU)

#define PG_ADDR(idx) ((u32)idx << 12)
#define PAGE_ALIGN(addr) ((u32)addr & 0xfffff000U)

#define ASSERT_PAGE(addr) kassert(((u32)addr & 0xfff) == 0)

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
} __packed ards_t;

typedef struct mem_info_t {
    u32 ards_count;
    ards_t *ards_arr;
    u32 max_zone_base;
    u32 max_zone_size;
} __packed mem_info_t;

#endif // !OAK_MM_MEMORY_H
