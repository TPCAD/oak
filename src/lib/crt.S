.code32

.section .text
.global _start

_start:
    xorl %ebp, %ebp
    popl %esi
    movl %esp, %ecx

    addl $-16, %esp
    pushl %eax
    pushl %esp
    pushl %edx
    pushl $_init
    pushl $_fini
    pushl %ecx
    pushl %esi
    pushl $main

    call __libc_start_main

    ud2
