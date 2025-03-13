#ifndef OAK_IDT_H
#define OAK_IDT_H

#include <oak/types.h>

#define IDT_SIZE 256
#define EXCEPTION_SIZE 0x20
#define ISR_SIZE 0x30

// dpl & 3 确保 dpl 不超过 3
#define IDT_ATTR(dpl) ((1 << 15) | (dpl & 3) << 13 | 0xe << 8)

typedef struct gate_desc {
    u32 low;
    u32 high;
} gate_desc;

void set_idt_entry(u32 index, u32 offset, u16 seg_selector, u8 dpl);
void idt_init();

#endif // !OAK_IDT_H
