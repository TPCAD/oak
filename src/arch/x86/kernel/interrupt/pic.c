#include "oak/debug/kassert.h"
#include <oak/interrupt/pic.h>
#include <oak/io.h>

// 通知中断控制器，中断处理结束
void pic_send_eoi(int vector) {
    if (vector >= 0x20 && vector < 0x28) {
        outb(PIC_M_CTRL, PIC_EOI);
    }
    if (vector >= 0x28 && vector < 0x30) {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

/**
 *  @brief  开关指定外中断
 *  @param  irq  外中断号
 *  @param  enable  开启或关闭
 */
void pic_set_intr_mask(u32 irq, bool enable) {
    kassert(irq >= 0 && irq < 16);
    u16 port = 0;
    if (irq < 8) {
        port = PIC_M_DATA;
    } else {
        port = PIC_S_CTRL;
        irq -= 8;
    }

    if (enable) {
        outb(port, inb(port) & ~(1 << irq));
    } else {
        outb(port, inb(port) | (1 << irq));
    }
}

/**
 *  @brief  初始化 PIC
 */
void pic_init() {
    // master chip init
    outb(PIC_M_CTRL, 0b00010001);    // ICW1: edge trigger,  cascade, need ICW4
    outb(PIC_M_DATA, IRQ_MASTER_NR); // ICW2: start interrupt vector
    outb(PIC_M_DATA, 0b00000100);    // ICW3: IR2 for slave
    outb(PIC_M_DATA, 0b00000001);    // ICW4: non-auto EOI, 8086 mode

    // slave chip init
    outb(PIC_S_CTRL, 0b00010001);   // ICW1: edge trigger,  cascade, need ICW4
    outb(PIC_S_DATA, IRQ_SLAVE_NR); // ICW2: start interrupt vector
    outb(PIC_S_DATA, 0b00000010);   // ICW3: IR2 for slave
    outb(PIC_S_DATA, 0b00000001);   // ICW4: non-auto EOI, 8086 mode

    // 屏蔽所有外中断
    outb(PIC_M_DATA, 0b11111111); // mask all interrupt in master
    outb(PIC_S_DATA, 0b11111111); // mask all interrupt in slave
}
