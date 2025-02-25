.code16
.global _start

_start:
    # 调用 BIOS 10 号中断，设置显示模式
    # 操作码 ah = 00h，参数 al = 03h
    movw $0x3, %ax
    int $0x10

    # 初始化段寄存器
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    # 初始化栈顶
    movw $0x7c00, %sp

    # 打印 "Booting..."
    movw $booting_msg, %si
    call print_str

    # 读取硬盘，加载 loader.s
    movw $DAPACK, %si # Disk Address Packet
    movb $0x42, %ah   # 操作码
    movb $0x80, %dl   # 硬盘号，0x80 是第一个硬盘
    int $0x13

    # 检验是否正确加载 loader.s
    cmpw $0x55aa, [0x1000]
    # 跳转 loader.s
    je 0x1002

    # 加载错误
    movw $error_msg, %si
    call print_str

    hlt

# 打印函数
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

booting_msg: .asciz "Booting...\n\r"
error_msg: .asciz "Booting error...\n\r"

DAPACK:
    .byte 0x10   # DAP Size，通常置为 16
    .byte 0      # 不使用，置 0
    .word 0x4    # 读取的扇区数量
    # 内存地址，segment:offset，x86 是小端序，如果分开定义 segment 和 offset，
    # 应该先定义 offset
    .word 0x1000 # offset
    .word 0      # segment
    # 读取的起始扇区，使用 LBA 地址，注意小端序
    .long 0x2
    .long 0

.fill 510-(.-_start), 1, 0
.word 0xaa55
