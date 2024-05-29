[bits 32]

multiboot_magic equ 0xe85250d6
i386 equ 0
length equ header_end - header_start

section .multiboot2

header_start:
	dd multiboot_magic ; magic number
	dd i386 ; 32-bit protected mode
	dd length ; header length
	dd -(multiboot_magic + i386 + length)

	dw 0 ; type
	dw 0 ; flag
	dw 8 ; size
header_end:

extern console_init
extern gdt_init
extern memory_init
extern kernel_init

section .text
global _start
_start:
	push ebx
	push eax
	
	call console_init
	call gdt_init
	call memory_init
	
	call kernel_init

	jmp $
