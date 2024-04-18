# BIOS

BIOS（Basic Input Output System）是一个固化在计算机主板上的程序，计算机通电后由
硬件自动加载到内存中，BIOS 是计算机启动的第一个程序。BIOS 的主要功能是对计算机
硬件进行检测及初始化。

## MBR

MBR（Master Boot Record），主引导记录或主引导扇区，通常是硬盘的第一个扇区，占
512 字节。有时候也将主引导扇区的前 446 字节叫做主引导记录。

MBR 结构：

- 0 ～ 439：引导程序代码
- 440 ～ 443：选用磁盘标志
- 444 ～ 445：一般为空
- 446 ～ 509：MBR 分区表，每个分区表为 16 字节
- 510 ～ 511：`0xaa55`。魔数，表示 MBR 分区结束

通常前 446 字节都可以用作引导程序。引导程序的主要功能是从硬盘等设备中将内核加载
到内存并为内核提供启动所需的参数。但 MBR 只有 512 字节，在大多数情况下并不足以
完成引导工作，所以大多数引导程序（如 Grub）都会再从硬盘加载另一个程序以引导内核。

完成检测后，BIOS 会从外部存储设备（硬盘、U盘等）的主引导扇区读取到内存中的
`0x7c00` 处，并执行该程序。

## BIOS 内存分布

BIOS 会对内存进行如下初始化：

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

因为 BIOS 运行在实模式下，所以寻址范围为 1 MB。

## BIOS 中断

内存的 `0x000 ～ 0x3FF` 是中断向量表区域，总共 1 KB。其中存储的是中断程序指针，
每个指针占 4 字节，因此总共有 256 个中断程序。

### 中断向量表

[BiOS interrupt call - WIKI](https://en.wikipedia.org/wiki/BIOS_interrupt_call)

### 示例

中断 `10h` 用于提供显示服务。

```asm
mov ah, 0x0e ; 电传打字机输出
mov al, '!' ; 输出字符 !
int 0x10 ; 调用中断
```


## 参考资料

- [StevenBaby - Github](https://github.com/StevenBaby/computer)
- [BiOS interrupt call - WIKI](https://en.wikipedia.org/wiki/BIOS_interrupt_call)
- [INT 10H - WIKI](https://en.wikipedia.org/wiki/INT_10H)
