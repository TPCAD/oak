#ifndef OAK_GLOBAL_H
#define OAK_GLOBAL_H

#include <oak/types.h>

#define GDT_SIZE 128

typedef struct descriptor_t {
    u16 limit_low;      // limit 0 ~ 15 bits
    u32 base_low : 24;  // base 0 ~ 23 bits
    u8 type : 4;        // access type
    u8 segment : 1;     // 0 for systemt segment, 1 for code or data segment
    u8 DPL : 2;         // descriptor privilege level
    u8 present : 1;     // 0 in disk, 1 in memory
    u8 limit_hight : 4; // limit 16 ~ 19
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

void gdt_init();

#endif // !OAK_GLOBAL_H
