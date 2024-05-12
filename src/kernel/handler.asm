[bits 32]

section .text

extern printk

global interrupt_handler

interrupt_handler:
	xchg bx, bx

	push message
	call printk
	add esp, 4

	xchg bx, bx
	iret

section .data

message:
	db "default interrupt" ,10, 0
