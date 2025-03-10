#include <oak/types.h>
#include <x86/gdt.h>

seg_desc gdt[GDT_SIZE];
u16 gdt_limit = sizeof(gdt) - 1;

void set_gdt_entry(u32 index, u32 base, u32 limit, u32 flags) {
    seg_desc *entry = &gdt[index];

    entry->low = SEG_BASE_L(base) | SEG_LIM_L(limit);
    entry->high =
        SEG_BASE_H(base) | flags | SEG_LIM_H(limit) | SEG_BASE_M(base);
}

void gdt_init() {
    set_gdt_entry(0, 0, 0, 0);
    set_gdt_entry(1, 0, 0xfffff, SEG_R0_CODE | BIT32);
    set_gdt_entry(2, 0, 0xfffff, SEG_R0_DATA | BIT32);
    set_gdt_entry(3, 0, 0xfffff, SEG_R3_CODE | BIT32);
    set_gdt_entry(4, 0, 0xfffff, SEG_R3_DATA | BIT32);
}
