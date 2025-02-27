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
    movw $0x0041, [0xb8000]
    hlt
