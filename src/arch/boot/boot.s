.code16
.global _start

_start:
	movw $0x3, %ax
	int $0x10

	movw $0, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0x7c00, %sp

	movw $msg, %si
	movb $0xe, %ah
print_char:
	lodsb
	cmpb $0, %al
	je done
	int $0x10
	jmp print_char

done:
	hlt

msg: .asciz "Booting..."

.fill 510-(.-_start), 1, 0
.word 0xaa55
