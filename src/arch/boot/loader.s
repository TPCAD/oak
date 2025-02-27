.code16
.global _start

# 魔数，用于校验是否正确加载
.word 0x55aa

_start:
    # 初始化段寄存器
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    # 初始化栈顶
    movw $0x1000, %sp

    # 打印 "Loading..."
    movw $loading_msg, %si
    call print_str

    # 打印 "Detecting memory..."
    movw $detecting_msg, %si
    call print_str

# 检测内存，检测结果存于 addrs_buffer
detect_memory:
    # 清空 ebx
    xorl %ebx, %ebx

    # es 置 0
    xorw %ax, %ax
    movw %ax, %es
    # edi 置为 ARDS 缓冲区指针
    movl $ards_buffer, %edi

    # 固定签名
    movl $0x534d4150, %edx

    .Ldetect:
        # 子功能号
        movl $0xe820, %eax
        # ards 结构大小
        movl $20, %ecx

        # 调用中断
        int $0x15

        # 错误处理
        jc .Ldetect_error

        # 将缓存指针指向下一个结构体
        addl %ecx, %edi

        # 结构体数量加 1
        incw [ards_count]

        # 判断是否为最后一个 ARDS
        cmpl $0, %ebx
        jnz .Ldetect

        # 打印 "success!"
        movw $success_msg, %si
        call print_str

        jmp enter_protected_mode

    .Ldetect_error:
        # 打印 "Error!"
        movw $error_msg, %si
        call print_str

        hlt

# 实模式打印字符
print_str:
    movb $0xe, %ah

    .Lprint_char:
        lodsb
        cmpb $0, %al
        je .Lprint_done
        int $0x10
        jmp .Lprint_char
    .Lprint_done:
        ret

enter_protected_mode:
    # 关中断
    cli

    # 开启 A20 线
    in $0x92, %al
    orb $2, %al
    out %al, $0x92

    # 加载 GDT
    lgdt [gdt_ptr]

    # 进入保护模式
    movl %cr0, %eax
    orb $1, %al
    movl %eax, %cr0

    # 进入保护模式后应立即进行一次远跳转以确保不会执行实模式指令
    ljmp $code_selector, $protected_mode

.code32
protected_mode:
    movw $data_selector, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $0x10000, %esp

    # 读取硬盘，加载内核
    movl $0x10000, %edi # 目标内存地址
    movl $10, %ecx      # 扇区起始地址（LBA28）
    movb $200, %bl      # 读取扇区数
    call read_disk

    # 跳转内核
    ljmp $code_selector, $0x10040

    hlt

read_disk:
    # 0x1f2 读取的扇区数
    movw $0x1f2, %dx
    movb %bl, %al
    outb %al, %dx

    # 0x1f3 LBA 0～7 位
    incw %dx
    movb %cl, %al
    outb %al, %dx

    # 0x1f4 LBA 8～15 位
    incw %dx
    shrl $8, %ecx
    movb %cl, %al
    outb %al, %dx

    # 0x1f5 LBA 16～23 位
    incw %dx
    shrl $8, %ecx
    movb %cl, %al
    outb %al, %dx

    # 0x1f6
    # - 0～3：起始扇区 LBA 地址 24～27 位
    # - 4：0 主驱动器，1 从驱动器
    # - 6：0 CHS 模式，1 LBA 模式
    # - 5、7：固定为 1
    incw %dx
    shrl $8, %ecx
    andb $0b1111, %cl

    movb $0b11100000, %al
    orb %cl, %al
    outb %al, %dx

    # 0x1f7 读硬盘命令
    incw %dx
    movb $0x20, %al
    outb %al, %dx

    # 以扇区数作为循环计数
    xorl %ecx, %ecx
    movb %bl, %cl
    .Lread_disk_read_sector:
        push %cx
        call .Lread_disk_wait
        call .Lread_disk_read
        pop %cx
        loop .Lread_disk_read_sector
        ret
    .Lread_disk_wait:
        movw $0x1f7, %dx
        .Lread_disk_wait_check:
            inb %dx, %al
            jmp .+2
            jmp .+2
            jmp .+2
            andb $0b10001000, %al
            cmpb $0b00001000, %al
            jnz .Lread_disk_wait_check
        ret
    .Lread_disk_read:
        movw $0x1f0, %dx
        movw $256, %cx
        .Lread_disk_read_read_word:
            inw %dx, %ax
            jmp .+2
            jmp .+2
            jmp .+2
            movw %ax, (%edi)
            addl $2, %edi
            loop .Lread_disk_read_read_word
        ret

loading_msg: .asciz "Loading...\n\r"
detecting_msg: .asciz "Detecting memory..."
error_msg: .asciz "Error!\n\r"
success_msg: .asciz "Success!\n\r"

# 段选择子
# | 3~15 | 2 | 0~1 |
#   ^      ^    ^
#   | 索引 |    | RPL
#          |
#          | 0 表示全局描述符，1表示局部描述符
.equ code_selector, (1 << 3) # 0b0000_0000_0000_1000
.equ data_selector, (2 << 3) # 0b0000_0000_0001_0000

# 32 位系统可访问的最大内存为 4G
.equ memory_base, 0 # 内存起始地址
# 内存结束地址，需要乘上 4K
.equ memory_limit, ((1024*1024*1024*4) / (1024*4)) - 1

# GDT 寄存器，前 16 位表示 GDT 大小，后 32 位表示 GDT 地址
gdt_ptr:
    .word (gdt_end - gdt_base) - 1
    .long gdt_base
# GDT，每一项 8 字节
gdt_base:
# 第一项必须是全 0
    .long 0, 0
# 代码段
gdt_code:
    # 段界限前 16 位
    .word memory_limit & 0xffff
    # 段基地址前 16 位
    .word memory_base & 0xffff
    # 段基地址 16～23 位
    .byte (memory_base >> 16) & 0xff
    # access type.
    # P: 1, on memory
    # DPL: 00, ring 0
    # S: 1, code or data segment
    # X: 1, code segment
    # C: 0, no conforming
    # R: 1, readable
    # A: 0, no accessed
    .byte 0b10011010
    # G: 1, 4K granularity
    # DB: 1, 32 bits
    # L: 0, not long-mode
    # AV: 0
    # 段界限后 4 位
    .byte 0b11000000 | ((memory_limit >> 16) & 0xf)
    # 段基地址 24～31 位
    .byte (memory_base >> 24) & 0xff
# 数据段
gdt_data:
    .word memory_limit & 0xffff
    .word memory_base & 0xffff
    .byte (memory_base >> 16) & 0xff
    # access type.
    # P: 1, on memory
    # DPL: 00, ring 0
    # S: 1, code or data segment
    # X: 0, data segment
    # D: 0, direction, grown up
    # W: 1, writable
    # A: 0, no accessed
    .byte 0b10010010
    .byte 0b11000000 | ((memory_limit >> 16) & 0xf)
    .byte (memory_base >> 24) & 0xff
gdt_end:

# Address Range Descriptor Structure
# 内存布局长度不定，应将缓冲区置于文件末，防止覆盖其他内容
ards_count: .byte 0
ards_buffer:
