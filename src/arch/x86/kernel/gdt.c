#include "oak/debug/kdebug.h"
#include <oak/gdt.h>
#include <oak/string.h>
#include <oak/types.h>

seg_desc gdt[GDT_SIZE];
u16 gdt_limit = sizeof(gdt) - 1;
tss_t tss;

void set_gdt_entry(u32 index, u32 base, u32 limit, u32 flags) {
    seg_desc *entry = &gdt[index];

    entry->low = SEG_BASE_L(base) | SEG_LIM_L(limit);
    entry->high =
        SEG_BASE_H(base) | flags | SEG_LIM_H(limit) | SEG_BASE_M(base);
}

void gdt_init() {
    set_gdt_entry(0, 0, 0, 0);
    set_gdt_entry(KERNEL_CODE_IDX, 0, 0xfffff, SEG_R0_CODE | BIT32);
    set_gdt_entry(KERNEL_DATA_IDX, 0, 0xfffff, SEG_R0_DATA | BIT32);
    set_gdt_entry(USER_CODE_IDX, 0, 0xfffff, SEG_R3_CODE | BIT32);
    set_gdt_entry(USER_DATA_IDX, 0, 0xfffff, SEG_R3_DATA | BIT32);
}

void tss_init() {
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.iobase = sizeof(tss);

    set_gdt_entry(KERNEL_TSS_IDX, (u32)&tss, sizeof(tss) - 1, SEG_TSS);

    BMB;
    asm volatile("ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));
}
