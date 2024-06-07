#ifndef OAK_MEMORY_H
#define OAK_MEMORY_H

#include <oak/types.h>

#define PAGE_SIZE 0x1000     // 4K per page
#define MEMORY_BASE 0x100000 // 1M, start address of free memory

#define KERNEL_MEMORY_SIZE 0x1000000 // memory occupied by kernel

#define KERNEL_BUFFER_MEM 0x800000
#define KERNEL_BUFFER_SIZE 0x400000

#define KERNEL_RAMDISK_MEM (KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE);
#define KERNEL_RAMDISK_SIZE 0x400000

#define USER_STACK_TOP 0x8000000 // user stack top address
#define USER_STACK_SIZE 0x200000
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

#define KERNEL_PAGE_DIR 0x1000 // page directory address

typedef struct page_entry_t {
    u8 present : 1;
    u8 write : 1;
    u8 user : 1;
    u8 pwt : 1;
    u8 pdt : 1;
    u8 accessed : 1;
    u8 dirty : 1;
    u8 pat : 1;
    u8 global : 1;
    u8 ignored : 3;
    u32 index : 20;
} _packed page_entry_t;

u32 get_cr2();
u32 get_cr3();
void set_cr3(u32 pde);

u32 alloc_kpage(u32 count);
void free_kpage(u32 vaddr, u32 count);

void link_page(u32 vaddr);
void unlink_page(u32 vaddr);

page_entry_t *copy_pde();

void free_pde();

int32 sys_brk(void *addr);
#endif // !OAK_MEMORY_H
