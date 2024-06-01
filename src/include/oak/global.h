#ifndef OAK_GLOBAL_H
#define OAK_GLOBAL_H

#include <oak/types.h>

#define GDT_SIZE 128

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2
#define KERNEL_TSS_IDX 3

#define USER_CODE_IDX 4
#define USER_DATA_IDX 5

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)
#define KERNEL_TSS_SELECTOR (KERNEL_TSS_IDX << 3)

#define USER_CODE_SELECTOR (USER_CODE_IDX << 3 | 0b11)
#define USER_DATA_SELECTOR (USER_DATA_IDX << 3 | 0b11)

typedef struct descriptor_t {
    u16 limit_low;      // limit 0 ~ 15 bits
    u32 base_low : 24;  // base 0 ~ 23 bits
    u8 type : 4;        // access type
    u8 segment : 1;     // 0 for systemt segment, 1 for code or data segment
    u8 DPL : 2;         // descriptor privilege level
    u8 present : 1;     // 0 in disk, 1 in memory
    u8 limit_high : 4;  // limit 16 ~ 19
    u8 available : 1;   // anything is ok
    u8 long_mode : 1;   // for x64 arch
    u8 big : 1;         // 0 for 16 bits, 1 for 32 bits
    u8 granularity : 1; // granularity, 0 for 4kiB, 1 for 1B
    u8 base_high;       // base 24 ~ 31 bits

} _packed descriptor_t;

typedef struct selector_t {
    u8 RPL : 2;
    u8 TI : 1; // 0 for GDT, 1 for LDT
    u16 index : 13;
} _packed selector_t;

typedef struct pointer_t {
    u16 limit; // size of GDT
    u32 base;  // offset/index of descriptor
} _packed pointer_t;

typedef struct tss_t {
    u32 backlink; // link of former task, save the selector of last tss
    u32 esp0;     // stack top address of ring0
    u32 ss0;      // stack selector of ring0
    u32 esp1;     // stack top address of ring1
    u32 ss1;      // stack selector of ring1
    u32 esp2;     // stack top address of ring2
    u32 ss2;      // stack selector of ring2
    u32 cr3;
    u32 eip;
    u32 flags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldtr;          // local descriptor selector
    u16 trace : 1;     // trigger debug exception when task switch if set 1
    u16 reversed : 15; // reserved
    u16 iobase;
    u32 ssp;
} tss_t;

void gdt_init();

#endif // !OAK_GLOBAL_H
