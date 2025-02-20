# Bootloader

Bootloader 是 BIOS 之后运行的第二个程序，负责加载操作系统内核，并在进入内核之前开启保护模式和长模式，检测内存等等。

Bootloader 通常分成两部分。BIOS 完成检测后会读取硬盘第一个扇区的内容到内存 `0x7c00` 处。因为只有一个扇区大小，这一部分的工作通常是读取硬盘，加载 Bootloader 的另一部分。

## Boot

这是 Bootloader 的第一部分，主要的内容就是利用 BIOS 中断读取硬盘然后跳转到另一部分。

## Loader

这是 Bootloader 的第二部分，也是 Bootloader 的核心部分。

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
 
| Type 值 | 名称                 | 描述                                                                                 |
| ------- | -------------------- | ------------------------------------------------------------------------------------ |
| 1       | AddressRangeMemory   | 这段内存可以被操作系统使用                                                           |
| 2       | AddressRangeReserved | 内存使用中或者被系统保留，操作系统不可以用此内存                                     |
| 3       |                      | 存储ACPI表，可以被操作系统回收。                                                     |
| 4       |                      | 操作系统不可使用这段内存。                                                           |
| 5       |                      | 已经损坏的内存区域，不可使用。                                                       |
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

实模式下只能访问最大 1 MB的内存，开启 A20 线后可以突破 1 MB。将 0x92 端口的第 2 位置 1 即可。

```assembly
    # 开启 A20 线
    in $0x92, %al
    orb $2, %al
    out %al, $0x92
```


### 长模式

## 参考资料

[Detecting Memory - OSDev](https://wiki.osdev.org/Detecting_Memory_(x86))
[X86 Assembly/Bootloader - Wikibooks/X86_Assembly](https://en.wikibooks.org/wiki/X86_Assembly/Bootloaders)
[A20 - OSDev](http://wiki.osdev.org/A20_Line)
[Protected Mode - OSDev](https://wiki.osdev.org/Protected_Mode)
