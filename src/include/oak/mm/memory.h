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
#define PAGE(idx) ((u32)idx << 12)

#define ASSERT_PAGE(addr) kassert((addr & 0xfff) == 0)

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
