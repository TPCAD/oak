#include <oak/debug/kassert.h>
#include <oak/debug/kdebug.h>
#include <oak/interrupt/idt.h>
#include <oak/interrupt/pic.h>
#include <oak/kprintf.h>

handler_t handler_table[IDT_SIZE];

static char *exception_msgs[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

/**
 *  @brief  注册中断处理函数
 *  @param  irq  外中断号
 *  @param  handler  中断处理函数
 */
void idt_set_intr_handler(u32 irq, handler_t handler) {
    kassert(irq >= 0 && irq < 16);
    handler_table[IRQ_MASTER_NR + irq] = handler;
}

/**
 *  @brief  非异常的 Intel 中断处理函数
 *  @param  vector  中断向量号
 */
void default_handler(u32 vector) {
    pic_send_eoi(vector);
    KDEBUG("%#x default interrupt called...\n", vector);
}

/**
 *  @brief  异常通用处理函数
 *  @param  vector  中断向量号
 */
void exception_handler(u32 vector, u32 edi, u32 esi, u32 ebp, u32 esp, u32 ebx,
                       u32 edx, u32 ecx, u32 eax, u32 gs, u32 fs, u32 es,
                       u32 ds, u32 vector0, u32 err_code, u32 eip, u32 cs,
                       u32 eflags) {
    char *msg = NULL;
    if (vector < 22) {
        msg = exception_msgs[vector];
    } else {
        msg = exception_msgs[15];
    }
    kprintf("\nEXCEPTION : %s \n", msg);
    kprintf("   VECTOR : 0x%02X\n", vector);
    kprintf("    ERROR : 0x%08X\n", err_code);
    kprintf("   EFLAGS : 0x%08X\n", eflags);
    kprintf("       CS : 0x%02X\n", cs);
    kprintf("      EIP : 0x%08X\n", eip);

    while (true) {
    }
}

extern void page_fault_handler();
void handler_init() {
    for (u32 i = 0; i < EXCEPTION_SIZE; i++) {
        handler_table[i] = exception_handler;
    }

    for (u32 i = EXCEPTION_SIZE; i < ISR_SIZE; i++) {
        handler_table[i] = default_handler;
    }
    
    handler_table[0xe]=page_fault_handler;
}
