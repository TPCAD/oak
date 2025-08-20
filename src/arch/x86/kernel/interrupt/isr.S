# handler.S 的主要功能是利用宏生成 ISR。
#
# ISR 不是 C 函数，因为 ISR 使用的是 `iret` 中断返回，而非普通的 `ret`。
#
# `iret` 只能处理不带错误码的情况，需要在 `iret` 执行前移动 `esp` 以跳过错误码。
#
# 但并不是所有中断都带有错误码，CPU 也不会为不带错误码的中断进行额外处理，因此
# 需要手动为不带错误码的中断压入一个值以保证栈的结构一致。
#
# 除此之外，还需要压入一个中断向量号来确定具体中断。
#
# 因此，一个完整的 ISR 应具有如下功能：
#
# 1. 中断发生时顺序入栈 `eflags`，`cs`，`eip`，错误码
# 2. 若中断没有错误码，则入栈一个值
# 3. 入栈中断向量号
# 4. 根据中断向量号执行具体的中断处理函数
# 5. 出栈中断向量号和错误码
# 6. `iret` 中断返回

.code32

.macro isr_template vector, no_error_code=1
.global isr\vector
isr\vector:
    # 为没有错误码的中断入栈一个值
    .if \no_error_code
        pushl $0x20240419
    .endif
    # 入栈中断向量号
    pushl $\vector
    jmp interrupt_entry
.endm

.section .text
.global interrupt_entry
.global interrupt_exit

interrupt_entry:
    # 调用 interrupt_handler，执行具体中断处理函数
    push %ds
    push %es
    push %fs
    push %gs
    pusha

    movl  48(%esp), %eax
    pushl %eax
    call *handler_table(,%eax,4)

interrupt_exit:
    add $4, %esp
    popa
    pop %gs
    pop %fs
    pop %es
    pop %ds

    # 出栈中断向量号和错误码
    add $8, %esp
    # 中断返回
    iret

# 使用宏生成 ISR
isr_template 0x00 # DIVISION ERROR
isr_template 0x01 # DEBUG
isr_template 0x02 # NON MASKABLE INTERRUPT
isr_template 0x03 # BREAKPOINT

isr_template 0x04 # OVERFLOW
isr_template 0x05 # BOUND RANGE EXCEED
isr_template 0x06 # INVALID OPCODE
isr_template 0x07 # NO MATH COPROCESSOR

isr_template 0x08 0 # DOUBLE FAULT
isr_template 0x09 # RESERVED
isr_template 0x0a 0 # INVALID TSS
isr_template 0x0b 0 # SEGMENT NOT PRESENT

isr_template 0x0c 0 # STACK SEGMENT FAULT
isr_template 0x0d 0 # GENERAL PROTECTION
isr_template 0x0e 0 # PAGE_FAULT
isr_template 0x0f # RESERVED

isr_template 0x10 # x87 FAULT
isr_template 0x11 0 # ALIGNMENT FAULT
isr_template 0x12 # MACHINE CHECK
isr_template 0x13 # SIMD FLOATING POINT FAULT

isr_template 0x14 # VIRTUALIZATION EXCEPTION
isr_template 0x15 # CONTROL PROTECTION
isr_template 0x16 # RESERVED
isr_template 0x17 # RESERVED

isr_template 0x18 # RESERVED
isr_template 0x19 # RESERVED
isr_template 0x1a # RESERVED
isr_template 0x1b # RESERVED

isr_template 0x1c # RESERVED
isr_template 0x1d # RESERVED
isr_template 0x1e # RESERVED
isr_template 0x1f # RESERVED

isr_template 0x20 # CLOCK
isr_template 0x21 # KEYBOARD
isr_template 0x22 # CASCADE
isr_template 0x23 # RESERVED

isr_template 0x24 # RESERVED
isr_template 0x25 # RESERVED
isr_template 0x26 # RESERVED
isr_template 0x27 # RESERVED

isr_template 0x28 # RESERVED
isr_template 0x29 # RESERVED
isr_template 0x2a # RESERVED
isr_template 0x2b # RESERVED

isr_template 0x2c # RESERVED
isr_template 0x2d # RESERVED
isr_template 0x2e # HARDDISK1 MASTER
isr_template 0x2f # HARDDISK2 SLAVE

# 记录 ISR 的入口地址，方便初始化 IDT
.section .data
.global isr_table
isr_table:
    .long isr0x00
    .long isr0x01
    .long isr0x02
    .long isr0x03
    .long isr0x04
    .long isr0x05
    .long isr0x06
    .long isr0x07
    .long isr0x08
    .long isr0x09
    .long isr0x0a
    .long isr0x0b
    .long isr0x0c
    .long isr0x0d
    .long isr0x0e
    .long isr0x0f
    .long isr0x10
    .long isr0x11
    .long isr0x12
    .long isr0x13
    .long isr0x14
    .long isr0x15
    .long isr0x16
    .long isr0x17
    .long isr0x18
    .long isr0x19
    .long isr0x1a
    .long isr0x1b
    .long isr0x1c
    .long isr0x1d
    .long isr0x1e
    .long isr0x1f
    .long isr0x20
    .long isr0x21
    .long isr0x22
    .long isr0x23
    .long isr0x24
    .long isr0x25
    .long isr0x26
    .long isr0x27
    .long isr0x28
    .long isr0x29
    .long isr0x2a
    .long isr0x2b
    .long isr0x2c
    .long isr0x2d
    .long isr0x2e
    .long isr0x2f


