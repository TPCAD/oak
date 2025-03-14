#include <oak/interrupt/idt.h>
#include <oak/interrupt/interrupt.h>
#include <oak/kdebug.h>
#include <oak/kprintf.h>

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
 *  @brief  非异常的 Intel 中断处理函数
 *  @param  vector  中断向量号
 */
void default_handler(u32 vector) {
    KDEBUG("%#x default interrupt called...\n", vector);
}

/**
 *  @brief  异常通用处理函数
 *  @param  vector  中断向量号
 *  @param  err_code  中断错误码
 *  @param  eip  eip 寄存器
 *  @param  cs  cs 寄存器
 *  @param  eflags  eflags 寄存器
 */
void exception_handler(u32 vector, u32 err_code, u32 eip, u32 cs, u32 eflags) {
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

/**
 *  @brief  中断处理函数
 *  @param  context  中断上下文
 *
 *  ISR 的核心部分，根据中断向量号选择具体的中断处理函数。中断上下文包括
 *  vector、err_code、eip、cs、eflags。
 */
void interrupt_handler(interrupt_context context) {
    if (context.vector < EXCEPTION_SIZE) {
        exception_handler(context.vector, context.err_code, context.eip,
                          context.cs, context.eflags);
    } else if (context.vector > EXCEPTION_SIZE && context.vector < ISR_SIZE) {
        default_handler(context.vector);
    }
    // TODO: 0xe page fault
    // TODO: 0x80 system call
    // TODO: other interrupts
}
