# BIOS

BIOS（Basic Input/Output System），基础输入输出系统，是在电脑通电阶段执行硬件初始化，以及为操作系统提供运行时服务的固件。

## BIOS 内存布局

| 起始地址  | 结束地址  | 大小     | 用途               |
| --------- | --------- | -------- | ------------------ |
| `0x000`   | `0x3FF`   | 1KB      | 中断向量表         |
| `0x400`   | `0x4FF`   | 256B     | BIOS 数据区        |
| `0x500`   | `0x7BFF`  | 29.75 KB | 可用区域           |
| `0x7C00`  | `0x7DFF`  | 512B     | MBR 加载区域       |
| `0x7E00`  | `0x9FBFF` | 607.6KB  | 可用区域           |
| `0x9FC00` | `0x9FFFF` | 1KB      | 扩展 BIOS 数据区   |
| `0xA0000` | `0xAFFFF` | 64KB     | 用于彩色显示适配器 |
| `0xB0000` | `0xB7FFF` | 32KB     | 用于黑白显示适配器 |
| `0xB8000` | `0xBFFFF` | 32KB     | 用于文本显示适配器 |
| `0xC0000` | `0xC7FFF` | 32KB     | 显示适配器 BIOS    |
| `0xC8000` | `0xEFFFF` | 160KB    | 映射内存           |
| `0xF0000` | `0xFFFEF` | 64KB-16B | 系统 BIOS          |
| `0xFFFF0` | `0xFFFFF` | 16B      | 系统 BIOS 入口地址 |

## BIOS 中断调用

BIOS 中断调用是 BIOS 提供的一组操作硬件的功能函数。

### 0x10

`0x10` 中断是 BIOS 提供的显示服务，可以设置显示模式，获取和设置光标位置，滚动窗口等等。

设置显示模式：

```language
    # 调用 BIOS 0x10 号中断，设置显示模式
    # 操作码 ah = 00h，参数 al = 03h
    movw $0x3, %ax
    int $0x10
```

打印字符：

```language
    # 调用 BIOS 0x10 号中断，设置显示模式
    # 操作码 ah = 0eh，参数 al = 字符
    movb $0x41, %al
    movw $0xe, %ax
    int $0x10
```

### 0x13

`0x13` 中断是 BIOS 提供的低级磁盘服务，可以读写硬盘等。

读取硬盘：

```language
    movw $DAPACK, %si # Disk Address Packet
    movb $0x42, %ah   # 操作码
    movb $0x80, %dl   # 硬盘号，0x80 是第一个硬盘
    int $0x13

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
```

## Reference

[INT 10H - Wikipedia](https://zh.wikipedia.org/wiki/INT_10H)
[Video Modes - Mendelson](https://mendelson.org/wpdos/videomodes.txt)

