# VGA

## 基本信息

在引导阶段使用 `int 0x10` 设置显示模式，BIOS 会在内存 `0x449` 处记录当前的显示模式。通常把显示模式设置为 16 色文本模式，分辨率为 80x25，每行 80 个 VGA 字符，共 25 行，对应的 `0x449` 值为 `0x03`。

VGA 彩色文字模式的显示内存地址为 `0xb8000`，大小为 32KB。

## VGA 字符

一个 VGA 字符占两个字节。低 8 位是字符，高 8 位是字符属性。字符属性见下表：

| Bit | Color | Function             |
| --- | ----- | ----------------     |
| 7   | B/I   | 闪烁或背景色亮色     |
| 6   | R     | 背景色               |
| 5   | G     | 背景色               |
| 4   | B     | 背景色               |
| 3   | I/CS  | 前景色亮色或字体选择 |
| 2   | R     | 前景色               |
| 1   | G     | 前景色               |
| 0   | B     | 前景色               |

第 7 位可由 Attribute Mode Control register 控制，默认是闪烁。第 3 位可由 Character Map Select register 控制，默认是前景色亮色。

16 色颜色如下表所示：

| I | R | G | B | Color         |
| - | - | - | - | ------------- |
| 0 | 0 | 0 | 0 | Black         |
| 0 | 0 | 0 | 1 | Blue          |
| 0 | 0 | 1 | 0 | Green         |
| 0 | 0 | 1 | 1 | Cyan          |
| 0 | 1 | 0 | 0 | Red           |
| 0 | 1 | 0 | 1 | Magenta       |
| 0 | 1 | 1 | 0 | Brown         |
| 0 | 1 | 1 | 1 | Light Gray    |
| 1 | 0 | 0 | 0 | Dark Gray     |
| 1 | 0 | 0 | 1 | Light Blue    |
| 1 | 0 | 1 | 0 | Light Green   |
| 1 | 0 | 1 | 1 | Light Cyan    |
| 1 | 1 | 0 | 0 | Light Red     |
| 1 | 1 | 0 | 1 | Light Magenta |
| 1 | 1 | 1 | 0 | Yellow        |
| 1 | 1 | 1 | 1 | White         |

## 访问 VGA 寄存器

VGA 的寄存器比它的 I/O 端口要多得多，因此 VGA 通过数据寄存器、地址寄存器和寄存器索引的组合来复用寄存器。

下面以访问 CRTC 寄存器为例介绍如何读写 VGA 寄存器。

CRTC 寄存器有一个 CRTC 地址寄存器和一个 CRTC 数据寄存器，对应的端口通常为 `0x3d4` 和 `0x3d5`。除此之外还有一系列下方的一系列寄存器：

- Index 00h -- Horizontal Total Register
- Index 01h -- End Horizontal Display Register
- Index 02h -- Start Horizontal Blanking Register
- Index 03h -- End Horizontal Blanking Register
- Index 04h -- Start Horizontal Retrace Register
- Index 05h -- End Horizontal Retrace Register
...
- Index 18h -- Line Compare Register

可以按下面的方法使用这些寄存器：

1. 将寄存器索引输出到 CRTC 地址寄存器；
2. 从 CRTC 数据寄存器读取数据或将数据写入 CRTC 数据寄存器；

```assembly
# 写 CRTC 地址寄存器
movw $0x3d4, %dx
movb $0xe, %al
outb %al, %dx

# 读 CRTC 数据寄存器
movw $0x3d5, %dx
inb %dx, %al
```

### CRTC 寄存器

#### Start Address High/Low Register（0xc、0xd）

这两个寄存器控制屏幕的起始位置，数值上等于屏幕左上角 VGA 字符的内存地址相对于内存基地址的 `偏移量的一半`。

#### Cursor Location High/Low Register（0xe、0xf）

这两个寄存器控制光标的位置，数值上等于光标所在 VGA 字符的内存地址相对于内存基地址的 `偏移量的一半`。

## 驱动实现
