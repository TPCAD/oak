[bits 32]

section .text

global inb
inb:
	; save stack frame
	push ebp
	mov ebp, esp

	xor eax, eax
	mov edx, [ebp + 8] ; take parameter from stack
	in al, dx
	
	; delay
	jmp $+2
	jmp $+2
	jmp $+2
	
	leave
	ret

global outb
outb:
	push ebp
	mov ebp, esp
	
	mov edx, [ebp + 8]
	mov eax, [ebp + 12]
	out dx, al

	; delay
	jmp $+2
	jmp $+2
	jmp $+2

	leave
	ret

global inw
inw:
	; save stack frame
	push ebp
	mov ebp, esp

	xor eax, eax
	mov edx, [ebp + 8] ; take parameter from stack
	in ax, dx
	
	; delay
	jmp $+2
	jmp $+2
	jmp $+2
	
	leave
	ret

global outw
outw:
	push ebp
	mov ebp, esp
	
	mov edx, [ebp + 8]
	mov eax, [ebp + 12]
	out dx, ax

	; delay
	jmp $+2
	jmp $+2
	jmp $+2

	leave
	ret
