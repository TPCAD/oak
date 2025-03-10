.code32

.equ multiboot_magic, 0xe85250d6
.equ length, multiboot_header_end - multiboot_header_start

.section .multiboot2

multiboot_header_start:
    .long multiboot_magic
    .long 0 # 32-bit protected mode for i386
    .long length
    .long -(multiboot_magic + 0 + length)
    .word 0
    .word 0
    .word 8
multiboot_header_end:

.section .text
.global _start
.align 16

_start:
    xchgw %bx, %bx

    call gdt_init
    subl $0x06, %esp
    movl $gdt, 2(%esp)
    movw gdt_limit, %ax
    movw %ax, (%esp)
    lgdt (%esp)
    addl $0x06, %esp

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    xchgw %bx, %bx
    pushw $0x08
    pushl $_after
    retf

_after:
    call kernel_init

    xchgw %bx, %bx
    hlt
