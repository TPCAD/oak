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

extern device_init
extern console_init
extern gdt_init
extern memory_init
extern kernel_init
extern gdt_ptr

code_selector equ (1 << 3)
data_selector equ (2 << 3)

section .text
global _start
_start:
	push ebx
	push eax
	
	call device_init
	call console_init
	
	call gdt_init

	lgdt [gdt_ptr]

	jmp dword code_selector:_next
_next:	

	mov ax, data_selector
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	call memory_init
	
	mov esp, 0x10000
	call kernel_init

	jmp $
