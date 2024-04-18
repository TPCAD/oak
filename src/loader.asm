[org 0x1000]

dw 0x55aa

xchg bx, bx

mov si, loading
call print

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

loading:
	db "Loading Oak...", 10 ,13, 0
