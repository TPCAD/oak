[org 0x7c00]

; set screen to text mode and clear screen
mov ax, 3
int 0x10

; initialize segment register
; it may go wrong on some virtual machine without initialize register
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00 ; initialize stack

xchg bx, bx

mov si, booting
call print

; halt
jmp $

/*
Invoke bios interruption 0x10 to dispaly text.
Pass 0x0e to ah as function code which teletype output mode,
pass output character to al, then use `int 0x10` to invoke interruption.
*/

print:
	mov ah, 0x0e
.next:
	mov al, [si]
	cmp al, 0
	jz .done
	int 0x10
	inc si
	jmp .next
.done:
	ret

booting:
	; 10 means \n, 13 means \r
	db "Booting Oak...", 10, 13, 0

; padding
times 510 - ($ - $$) db 0

; MBR magic number
dw 0xaa55
