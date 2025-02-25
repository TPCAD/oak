# Bootloader

在传统的 BIOS 引导 + MBR 分区的组合中，BIOS 在完成硬件检查后会加载主引导记录（Master Boot Record，又叫主引导扇区），即硬盘的第一个扇区到内存 `0000:7c00h` 处，然后将控制权交出。因此，bootloader 通常存储在主引导扇区中。

主引导扇区只有 512 字节，而 bootloader 需要完成开启保护模式和长模式，检测内存，加载操作系统内核等等工作，因此 bootloader 通常会被分成两部分。

## Boot

Bootloader 的第一部分，主要功能是利用 BIOS 中断读取硬盘然后跳转到另一部分。

主引导扇区必须以 `0x55aa` 结尾，即 512 字节的最后两个字节必须是 `0x55aa`，又因为 Intel 的 CPU 是小端序，所以结尾的两字节是 `0xaa55`。

## Loader

Bootloader 的第二部分，也是 Bootloader 的核心部分。

### 检测内存

目前最常用的方法是使用 BIOS 0x15 号中断。

#### ARDS

内存范围描述符结构体（Address Range Descriptor Structure）是 0x15 中断返回的用于描述内存的结构体，大小通常为 20 字节。

| 字节偏移量 | 属性名称     | 描述                             |
| ---------- | ------------ | -------------------------------- |
| 0          | BaseAddrLow  | 基地址的低 32 位                 |
| 4          | BaseAddrHigh | 基地址的高 32 位                 |
| 8          | LengthLow    | 内存长度的低 32 位，以字节为单位 |
| 12         | LengthHigh   | 内存长度的高 32 位，以字节为单位 |
| 16         | Type         | 本段内存的类型                   |

#### Type 字段
 
| Type 值 | 名称                 | 描述                                                                     |
| ------- | -------------------- | ------------------------------------------------------------------------ |
| 1       | AddressRangeMemory   | 这段内存可以被操作系统使用                                               |
| 2       | AddressRangeReserved | 内存使用中或者被系统保留，操作系统不可以用此内存                         |
| 3       |                      | 存储ACPI表，可以被操作系统回收。                                         |
| 4       |                      | 操作系统不可使用这段内存。                                               |
| 5       |                      | 已经损坏的内存区域，不可使用。                                           |
| 其他    | 未定义               | 未定义，将来会用到，目前保留，但是需要操作系统一样将其视为ARR(AddressRangeReserved)  |

#### 调用参数

| 寄存器或状态位   | 描述                                |
| ---------------- | ----------------------------------- |
| eax              | 子功能号，0xe820                    |
| ebx              | 初次使用时置 0                      |
| es:di            | ARDS 缓冲区指针                     |
| ecx              | ARDS 大小（字节），通常为 20 字节   |
| edx              | 固定签名 `0x534d4150`               |

#### 返回值

| 寄存器或状态位   | 描述                                                 |
| ---------------- | ---------------------------------------------------- |
| CF               | 置 0 则未出错，置 1 则出错                           |
| eax              | 0x534d4150                                           |
| es:di            | ARDS 缓冲区指针，与输入参数相同                      |
| ecx              | ARDS 大小（字节），通常为 20 字节                    |
| ebx              | 后续值，即下一个 ARDS 的位置，每次调用都会更新这个值 |

若 CF 为 0，ebx 为 0，则表示这是最后一个 ARDS。

#### 使用 0x15 中断

```assembly
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

# Address Range Descriptor Structure
# 内存布局长度不定，应将缓冲区置于文件末，防止覆盖其他内容
ards_count: .byte 0
ards_buffer:
```

### 保护模式

进入保护模式前需要先完成以下步骤：

1. 关闭中断
2. 开启 A20 线
3. 加载 GDT

#### A20 线

实模式下只能访问最大 1 MB的内存，开启 A20 线后可以突破 1 MB。有许多方法开启 A20 线，下面是最简单的一种。

```assembly
    # 开启 A20 线
    in $0x92, %al
    orb $2, %al
    out %al, $0x92
```

#### 加载 GDT

```assembly
# 加载 GDT
lgdt [gdt_ptr]

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
.equ memory_limit, ((1024*1024*1024*4) / (1024*4) - 1)

# GDT 寄存器，前 16 位表示 GDT 大小，后 32 位表示 GDT 地址
gdt_ptr:
    .word (gdt_end - gdt_base) - 1
    .long gdt_base
# GDT，每一项 8 字节
gdt_base:
# 第一项必须是全 0
    .long 0, 0
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
    .byte 0b11000000 | (memory_limit >> 16) & 0xf
    # 段基地址 24～31 位
    .byte (memory_base >> 24) & 0xff
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
    .byte 0b11000000 | (memory_limit >> 16) & 0xf
    .byte (memory_base >> 24) & 0xff
gdt_end:
```

#### 进入保护模式

```assembly
enter_protected_mode:
    # 关中断
    cli

    # 开启 A20 线
    # ...

    # 进入保护模式
    movl %cr0, %eax
    orb $1, %al
    movl %eax, %cr0

    # 进入保护模式后应立即进行一次远跳转以确保不会执行实模式指令
    ljmp $code_selector, $protected_mode

.code32
protected_mode:
    # 设置段地址
    movw $data_selector, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
```

#### 加载内核

### 长模式

## 参考资料

[Detecting Memory - OSDev](https://wiki.osdev.org/Detecting_Memory_(x86))
[X86 Assembly/Bootloader - Wikibooks/X86_Assembly](https://en.wikibooks.org/wiki/X86_Assembly/Bootloaders)
[A20 - OSDev](http://wiki.osdev.org/A20_Line)
[Protected Mode - OSDev](https://wiki.osdev.org/Protected_Mode)
