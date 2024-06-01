#include <oak/debug.h>
#include <oak/global.h>
#include <oak/string.h>

descriptor_t gdt[GDT_SIZE];
pointer_t gdt_ptr;
tss_t tss;

void descriptor_init(descriptor_t *desc, u32 base, u32 limit) {
    desc->base_low = base & 0xffffff;
    desc->base_high = (base >> 24) & 0xff;
    desc->limit_low = limit & 0xffff;
    desc->limit_high = (limit >> 16) & 0xf;
}

void gdt_init() {
    DEBUGK("init GDT!\n");

    memset(gdt, 0, sizeof(gdt));

    descriptor_t *desc;

    desc = gdt + KERNEL_CODE_IDX;
    descriptor_init(desc, 0, 0xfffff);
    desc->segment = 1;     // code segment
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 bits
    desc->long_mode = 0;   // not 64 bits
    desc->present = 1;     // in memory
    desc->DPL = 0;         // kernel privilege
    desc->type = 0b1010;   // code, up, writable, not accessed

    desc = gdt + KERNEL_DATA_IDX;
    descriptor_init(desc, 0, 0xfffff);
    desc->segment = 1;     // code segment
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 bits
    desc->long_mode = 0;   // not 64 bits
    desc->present = 1;     // in memory
    desc->DPL = 0;         // kernel privilege
    desc->type = 0b0010;   // data, up, writable, not accessed

    desc = gdt + USER_CODE_IDX;
    descriptor_init(desc, 0, 0xfffff);
    desc->segment = 1;     // code segment
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 bits
    desc->long_mode = 0;   // not 64 bits
    desc->present = 1;     // in memory
    desc->DPL = 3;         // user privilege
    desc->type = 0b1010;   // code, up, writable, not accessed

    desc = gdt + USER_DATA_IDX;
    descriptor_init(desc, 0, 0xfffff);
    desc->segment = 1;     // code segment
    desc->granularity = 1; // 4K
    desc->big = 1;         // 32 bits
    desc->long_mode = 0;   // not 64 bits
    desc->present = 1;     // in memory
    desc->DPL = 3;         // user privilege
    desc->type = 0b0010;   // data, up, writable, not accessed

    gdt_ptr.base = (u32)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;
}

void tss_init() {
    memset(&tss, 0, sizeof(tss));

    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.iobase = sizeof(tss);

    descriptor_t *desc = gdt + KERNEL_TSS_IDX;
    descriptor_init(desc, (u32)&tss, sizeof(tss) - 1);
    desc->segment = 0;     // system segment
    desc->granularity = 0; // byte
    desc->big = 0;         // fixed 0
    desc->long_mode = 0;   // fixed 0
    desc->present = 1;     // in memory
    desc->DPL = 0;         // task gate or call gate
    desc->type = 0b1001;   // 32 bits tss

    asm volatile("ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));
}
