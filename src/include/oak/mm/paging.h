#ifndef OAK_MM_PAGING_H
#define OAK_MM_PAGING_H

#include <oak/types.h>

#define PG_PRESENT (0x1)
#define PG_WRITE (0x1 << 1)
#define PG_USER (0x1 << 2)
#define PG_WRITE_THROUGH (0x1 << 3)
#define PG_CACHE_DISABLE (0x1 << 4)
#define PG_PDE_4MB (0x1 << 7)

#define PG_ATTR_P (PG_PRESENT)
#define PG_ATTR_PW (PG_PRESENT | PG_WRITE)
#define PG_ATTR_PWU (PG_PRESENT | PG_WRITE | PG_USER)

#define PD_BASE_VADDR 0xfffff000U // 页目录的虚拟基地址
#define PT_BASE_VADDR 0xffc00000U // 页表的虚拟基地址

typedef u32 page_entry_t;

void paging_init();

#endif // !OAK_MM_PAGING_H
