#ifndef OAK_INTERRUPT_H
#define OAK_INTERRUPT_H

#include <oak/types.h>

#define IDT_SIZE 256

typedef struct gate_t {
    u16 offset_low;  // offset 0 ~ 15 bits
    u16 selector;    // segment selector
    u8 reserved;     // reserved
    u8 type : 4;     // 0b1110 presents interrupt gate
    u8 segment : 1;  // 0 for system segment
    u8 DPL : 2;      // Descriptor Privilege Level
    u8 present : 1;  // must be 1 to be valid
    u16 offset_high; // offset 16 ~ 31 bits
} _packed gate_t;

void interrupt_init();

#endif // !OAK_INTERRUPT_H
