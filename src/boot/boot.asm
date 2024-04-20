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

mov si, booting
call print

; read disk to load loader.asm
call read_disk

; check if load rightly
cmp word [0x1000], 0x55aa
jnz error

; jump to loader.asm
jmp 0:0x1002

; halt
jmp $

read_disk:
	mov si, DAPACK
	mov ah, 0x42 ; function number
	mov dl, 0x80 ; first hard drive
	int 0x13
	ret
	
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

; error handle
error:
	mov si, .msg
	call print
	hlt
	jmp $
.msg:
	db "Booting error...", 13, 10, 0

booting:
	; 10 means \n, 13 means \r
	db "Booting Oak...", 10, 13, 0
	
DAPACK:
	db 0x10 ; size of packet (16 bytes)
	db 0 ; always 0
	dw 4 ; number of sectors to read
	dw 0x1000 ; buffer offset address
	dw 0 ; buffer segment address
	dd 2 ; lower 32 bits of LBA address
	dd 0 ; upper 16 bits of LBA address

; padding
times 510 - ($ - $$) db 0

; MBR magic number
dw 0xaa55
