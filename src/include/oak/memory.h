#ifndef OAK_MEMORY_H
#define OAK_MEMORY_H

#include <oak/types.h>

#define PAGE_SIZE 0x1000 // 4K per page
// The largest memory address which can be used by bios is 1M.
// Therefore 1M is the start address of free memory,
#define MEMORY_BASE 0x100000

#define KERNEL_MEMORY_SIZE 0x1000000 // memory occupied by kernel

#define KERNEL_BUFFER_MEM 0x800000
#define KERNEL_BUFFER_SIZE 0x400000

#define KERNEL_RAMDISK_MEM (KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE)
#define KERNEL_RAMDISK_SIZE 0x400000

#define USER_STACK_TOP 0x10000000 // user stack top address, 256M
#define USER_STACK_SIZE 0x200000  // 2M
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

#define USER_EXEC_ADDR 0x1000000 // 16M

#define USER_MMAP_ADDR 0x8000000 // 128M
#define USER_MMAP_SIZE 0x8000000

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
    u8 shared : 1;
    u8 private : 1;
    u8 readonly : 1;
    u32 index : 20;
} _packed page_entry_t;

u32 get_cr2();
u32 get_cr3();
void set_cr3(u32 pde);

u32 alloc_kpage(u32 count);
void free_kpage(u32 vaddr, u32 count);

page_entry_t *get_entry(u32 vaddr, bool create);

void flush_tlb(u32 vaddr);

void link_page(u32 vaddr);
void unlink_page(u32 vaddr);

page_entry_t *copy_pde();

void free_pde();
#endif // !OAK_MEMORY_H
