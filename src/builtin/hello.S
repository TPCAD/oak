.code32

.section .text
.global _start

_start:
    movl $4, %eax
    movl $1, %ebx
    movl $message, %ecx
    movl $message_end - message - 1, %edx
    int $0x80

    movl $1, %eax
    movl $0, %ebx
    int $0x80
.size _start, . - _start

.section .data

message:
    .asciz "hello oak!\n"
message_end:
.size message, message_end - message

.section .bss
buffer: .skip 1024
