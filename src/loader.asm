[org 0x1000]

dw 0x55aa

mov si, loading
call print

call detect_memory

jmp 0:pre_protected_mode

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

pre_protected_mode:

	; disable interruption
	cli

	; turn on A20 line
	in al, 0x92
	or al, 0b10
	out 0x92, al

	; load GDT
	lgdt [gdt_ptr]

	; launch protected mode
	mov eax, cr0
	or al, 1
	mov cr0, eax

	; a far jump after launch immediately
	jmp dword code_selector:protect_mode

[bits 32]
protect_mode:

	; initialize segment register
	mov ax, data_selector
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov esp, 0x10000

	mov byte [0xb8000], 'P'

jmp $

loading:
	db "Loading Oak...", 10 ,13, 0
	
detecting:
	db "Detecting memory success...", 10 ,13, 0

; GDT related

; top 3 bits are 0
code_selector equ (1 << 3)
data_selector equ (2 << 3)

memory_base equ 0
; 0xfffff
memory_limit equ ((1024 * 1024 * 1024 * 4) / (1024 * 4)) - 1

gdt_ptr:
	dw (gdt_end - gdt_base) - 1
	dd gdt_base
gdt_base:
	dd 0,0 ; first segment descriptor must be all 0
gdt_code:
	dw memory_limit & 0xffff ; limit 0~15 bits
	dw memory_base & 0xffff ; base 0~15 bits
	db (memory_base >> 16) & 0xff ; base 16~24 bits
	; access type.
	; P: 1, on memory
	; DPL: 00, ring 0
	; S: 1, code or data segment
	; X: 1, code segment
	; C: 0, no conforming
	; R: 1, readable
	; A: 0, no accessed
	db 0b1_00_1_1010
	; G: 1, 4K granularity
	; DB: 1, 32 bits
	; L: 0, not long-mode
	; AV: 0
	; limit 16~19 bits only has 4 bits, so it must be placed with flags
	db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
	db (memory_base >> 24) & 0xff
gdt_data:
	dw memory_limit & 0xffff ; limit 0~15 bits
	dw memory_base & 0xffff ; base 0~15 bits
	db (memory_base >> 16) & 0xff ; base 16~24 bits
	; access type.
	; P: 1, on memory
	; DPL: 00, ring 0
	; S: 1, code or data segment
	; X: 0, data segment
	; D: 0, direction, grown up
	; W: 1, writable
	; A: 0, no accessed
	db 0b1_00_1_0010
	; G: 1, 4K granularity
	; DB: 1, 32 bits
	; L: 0, not long-mode
	; AV: 0
	; limit 16~19 bits only has 4 bits, so it must be placed with flags
	db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
	db (memory_base >> 24) & 0xff
gdt_end:

ards_count:
	dw 0
ards_buffer:
