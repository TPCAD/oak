#ifndef OAK_GDT_H
#define OAK_GDT_H

#include <oak/types.h>

#define SD_TYPE(x) (x << 8)
#define SD_CODE_DATA(x) (x << 12)
#define SD_DPL(x) (x << 13)
#define SD_PRESENT(x) (x << 15)
#define SD_AVL(x) (x << 20)
#define SD_64BITS(x) (x << 21)
#define SD_32BITS(x) (x << 22)
#define SD_4K_GRAN(x) (x << 23)

#define SEG_LIM_L(x) (x & 0x0ffff)
#define SEG_LIM_H(x) (x & 0xf0000)

#define SEG_BASE_L(x) ((x & 0x0000ffff) << 16)
#define SEG_BASE_M(x) ((x & 0x00ff0000) >> 16)
#define SEG_BASE_H(x) (x & 0xff000000)

#define SEG_DATA_RD 0x00        // Read-Only, expand-up, not-accessed
#define SEG_DATA_RDA 0x01       // Read-Only, expand-up, accessed
#define SEG_DATA_RDWR 0x02      // Read/Write, expand-up, not-accessed
#define SEG_DATA_RDWRA 0x03     // Read/Write, expand-up, accessed
#define SEG_DATA_RDEXPD 0x04    // Read-Only, expand-down, not-accessed
#define SEG_DATA_RDEXPDA 0x05   // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD 0x06  // Read/Write, expand-down, not-accessed
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX 0x08        // Execute-Only, no-conforming, not-accessed
#define SEG_CODE_EXA 0x09       // Execute-Only, no-conforming, accessed
#define SEG_CODE_EXRD 0x0a      // Execute/Write, no-conforming, not-accessed
#define SEG_CODE_EXRDA 0x0b     // Execute/Write, no-conforming, accessed
#define SEG_CODE_EXC 0x0c       // Execute-Only, conforming, not-accessed
#define SEG_CODE_EXCA 0x0d      // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC 0x0e     // Execute/Write, conforming, not-accessed
#define SEG_CODE_EXRDCA 0x0f    // Execute/Write, conforming, accessed

#define SEG_TSS_TYPE 0x9

#define BIT64 (SD_64BITS(1) | SD_32BITS(0))
#define BIT32 (SD_64BITS(0) | SD_32BITS(1))

#define SEG_R0_CODE                                                            \
    SD_TYPE(SEG_CODE_EXRD) | SD_CODE_DATA(1) | SD_DPL(0) | SD_PRESENT(1) |     \
        SD_AVL(0) | SD_4K_GRAN(1)

#define SEG_R0_DATA                                                            \
    SD_TYPE(SEG_DATA_RDWR) | SD_CODE_DATA(1) | SD_DPL(0) | SD_PRESENT(1) |     \
        SD_AVL(0) | SD_4K_GRAN(1)

#define SEG_R3_CODE                                                            \
    SD_TYPE(SEG_CODE_EXRD) | SD_CODE_DATA(1) | SD_DPL(3) | SD_PRESENT(1) |     \
        SD_AVL(0) | SD_4K_GRAN(1)

#define SEG_R3_DATA                                                            \
    SD_TYPE(SEG_DATA_RDWR) | SD_CODE_DATA(1) | SD_DPL(3) | SD_PRESENT(1) |     \
        SD_AVL(0) | SD_4K_GRAN(1)

#define SEG_TSS                                                                \
    SD_TYPE(SEG_TSS_TYPE) | SD_CODE_DATA(0) | SD_DPL(0) | SD_PRESENT(1) |      \
        SD_AVL(0) | SD_4K_GRAN(0)

#define GDT_SIZE 6

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2
#define KERNEL_TSS_IDX 3
#define USER_CODE_IDX 4
#define USER_DATA_IDX 5

/*
 *
 *  段选择器（Selector）类似实模式中的段地址，它指明了段描述符在 GDT
 *  中的偏移位置，还带有校验功能。 结构：
 *  - 0～1：RPL
 *  - 2：0 表示全局描述符，1 表示局部描述符
 *  - 3～15：GDT 索引
 */
#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)
#define KERNEL_TSS_SELECTOR (KERNEL_TSS_IDX << 3)
#define USER_CODE_SELECTOR (USER_CODE_IDX << 3 | 0b11) // RPL = 3
#define USER_DATA_SELECTOR (USER_DATA_IDX << 3 | 0b11)

typedef struct seg_desc {
    u32 low;
    u32 high;
} seg_desc;

typedef struct tss_t {
    u32 backlink; // link of former task, save the selector of last tss
    u32 esp0;     // ring0 的栈顶地址
    u32 ss0;      // ring0 的栈段选择子
    u32 esp1;     // ring1 的栈顶地址
    u32 ss1;      // ring1 的栈段选择子
    u32 esp2;     // ring2 的栈顶地址
    u32 ss2;      // ring2 的栈段选择子
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
    u32 ldtr;          // 局部描述符选择子
    u16 trace : 1;     // trigger debug exception when task switch if set 1
    u16 reversed : 15; // reserved
    u16 iobase;
    u32 ssp;
} tss_t;

void gdt_init();
void set_gdt_entry(u32 index, u32 base, u32 limit, u32 flags);

#endif // !OAK_GDT_H
