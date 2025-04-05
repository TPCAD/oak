.code32

.section .text

.globl syscall_isr
syscall_isr:
    # 检查系统调用号是否在范围内
    pushl %eax
    call syscall_check
    addl $4, %esp

    # 入栈错误码
    pushl $0x20240419

    # 入栈中断向量号
    pushl $0x80

    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    pushal

    pushl $0x80

    # 入栈系统调用参数
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx

    # 调用对应的系统调用函数
    call *syscall_table(,%eax,4)

    # 出栈系统调用参数
    addl $24, %esp

    # 将返回值写入到先前入栈的 %eax 的位置，在 popa 后 %eax 就保存了系统调用的返回值
    movl %eax, 32(%esp)

    # 中断返回
    # 出栈中断向量号
    add $4, %esp
    popa
    pop %gs
    pop %fs
    pop %es
    pop %ds

    # 出栈中断向量号和错误码
    add $8, %esp
    # 中断返回
    iret
