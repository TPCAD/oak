[org 0x1000]

dw 0x55aa

mov si, loading
call print

xchg bx, bx

call detect_memory

jmp $

detect_memory:
	xor ebx, ebx

	mov ax, 0
	mov es, ax

	mov edi, ards_buffer
	mov edx, 0x534d4150

.next:
	mov eax, 0xe820
	mov ecx, 20
	int 0x15

	jc error

	add di, cx
	inc word [ards_count]

	cmp ebx, 0
	jnz .next

	mov si, detecting
	call print

	ret
	
; 	mov cx, [ards_count]
; 	mov si, 0
;
; .show:
; 	mov eax, [ards_buffer + si]
; 	mov ebx, [ards_buffer + si + 8]
; 	mov edx, [ards_buffer + si + 16]
; 	add si, 20
; 	
; 	xchg bx, bx
; 	loop .show


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

error:
	mov si, .msg
	call print
	hlt
	jmp $
.msg:
	db "Loading error...", 13, 10, 0

loading:
	db "Loading Oak...", 10 ,13, 0
	
detecting:
	db "Detecting memory success...", 10 ,13, 0

ards_count:
	dw 0
ards_buffer:
