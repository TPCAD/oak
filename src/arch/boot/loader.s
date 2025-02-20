.code16
.global _start

.word 0x55aa

_start:
    xchgw %bx, %bx

    # 初始化段寄存器
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    # 初始化栈顶
    movw $0x1000, %sp

    movw $loading_msg, %si
    call print_str

    movw $detecting_msg, %si
    call print_str

# 检测内存，检测结果存于 addrs_buffer
detect_memory:
    # 清空 ebx
    xorl %ebx, %ebx

    # es 置 0
    xorw %ax, %ax
    movw %ax, %es
    # di 置为缓存位置
    movl $ards_buffer, %edi

    # 固定签名
    movl $0x534d4150, %edx

detect:
    # 子功能号
    movl $0xe820, %eax
    # ards 结构大小
    movl $20, %ecx

    int $0x15

    # 错误处理
    jc detect_error

    # 将缓存指针指向下一个结构体
    addl %ecx, %edi

    # 结构体数量加 1
    incw [ards_count]

    cmpl $0, %ebx
    jnz detect

    movw $success_msg, %si
    call print_str

    jmp enter_protected_mode

detect_error:
    movw $error_msg, %si
    call print_str

    hlt

enter_protected_mode:

    # 开启 A20 线
    xchgw %bx, %bx
    in $0x92, %al
    orb $2, %al
    out %al, $0x92

    # 加载 GDT
    # xchgw %bx, %bx
    lgdt [gdt_ptr]

    movl %cr0, %eax
    orb $1, %al
    movl %eax, %cr0

    xchgw %bx, %bx
    ljmp $0x80, $protected_mode

protected_mode:
    xchgw %bx, %bx
    movw $data_selector, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # movl $0x10000, %esp

    xchgw %bx, %bx
    hlt

# 打印字符
print_str:
    movb $0xe, %ah
print_char:
    lodsb
    cmpb $0, %al
    je print_done
    int $0x10
    jmp print_char
print_done:
    ret

loading_msg: .asciz "Loading...\n\r"
detecting_msg: .asciz "Detecting memory..."
error_msg: .asciz "Error!\n\r"
success_msg: .asciz "Success!\n\r"

# 段选择符，0~1 位表示 RPL，2 位 0 表示全局描述符，1 表示本地描述符
# 剩下表示在描述表中的索引
.equ code_selector, (1 << 3)
.equ data_selector, (2 << 3)

# 32 位最多表示 4G 内存
.equ memory_base, 0
.equ memory_limit, ((1024*1024*1024*4) / (1024*4) - 1)

gdt_ptr:
    .word (gdt_end - gdt_base) - 1
    .long gdt_base
gdt_base:
    .long 0, 0
gdt_code:
    .word memory_limit & 0xffff
    .word memory_base & 0xffff
    .byte (memory_base >> 16) & 0xff
    .byte 0b10011010
    .byte 0b11000000 | (memory_limit >> 16) & 0xf
    .byte (memory_base >> 24) & 0xff
gdt_data:
    .word memory_limit & 0xffff
    .word memory_base & 0xffff
    .byte (memory_base >> 16) & 0xff
    .byte 0b10010010
    .byte 0b11000000 | (memory_limit >> 16) & 0xf
    .byte (memory_base >> 24) & 0xff
gdt_end:

# Address Range Descriptor Structure
# 内存布局长度不定，应将缓冲区置于文件末，防止覆盖其他内容
ards_count: .byte 0
ards_buffer:
