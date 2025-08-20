#include <oak/interrupt/idt.h>

/**
 *  isr_entry_table 来自 handler.S，记录了每个 ISR 的入口地址。
 */
extern u32 isr_table[ISR_SIZE];
extern void syscall_isr();

gate_desc_t idt[IDT_SIZE];
u16 idt_limit = sizeof(idt) - 1;

/**
 *  @brief  设置中断描述符
 *  @param  index  中断向量号
 *  @param  offset  isr 地址
 *  @param  seg_selector  段选择子
 *  @param  dpl  DPL
 */
void set_idt_entry(u32 index, u32 offset, u16 seg_selector, u8 dpl) {
    gate_desc_t *ptr = &idt[index];
    ptr->low = seg_selector << 16 | (offset & 0x0000ffff);
    ptr->high = (offset & 0xffff0000) | IDT_ATTR(dpl);
}

/**
 *  @brief  初始化 IDT
 */
void idt_init() {
    for (int i = 0; i < ISR_SIZE; i++) {
        set_idt_entry(i, isr_table[i], 0x08, 0);
    }
    handler_init();

    set_idt_entry(0x80, (u32)syscall_isr, 0x08, 3);
}
