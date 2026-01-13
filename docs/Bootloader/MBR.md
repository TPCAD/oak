Master Boot Record（MBR），主引导记录或主引导扇区，是一种传统的硬盘分区方案，它将分区表和引导程序存储在硬盘的第一个扇区。BIOS 在完成加电自检后会将该扇区加载到内存 `0x7c00` 处。MBR 通常也指代该扇区。

## MBR 结构

下面是一种通用的 MBR 结构，更多结构可以在[这里](https://en.wikipedia.org/wiki/Master_boot_record#Sector_layout)找到：

| Offset | Size(bytes) | Description                        |
| ------ | ----------- | ---------------------------------- |
| 0x000  | 440         | MBR 引导程序                       |
| 0x1B8  | 4           | Unique Disk ID/Signature(Optional) |
| 0x1BC  | 2           | Reserved 0x0000(Optional)          |
| 0x1BE  | 16          | 分区表第一项                       |
| 0x1CE  | 16          | 分区表第二项                       |
| 0x1DE  | 16          | 分区表第三项                       |
| 0x1EE  | 16          | 分区表第四项                       |
| 0x1FE  | 2           | 0x55AA                             |

MBR 要求扇区最后两字节必须是 `0x55AA`，用于校验该扇区是否有效引导扇区。除去分区表 64 字节和有效签名 2 字节，剩下的 446 字节就是引导程序可以使用最大空间（包括可选的 6 字节），这对于一个引导程序来说往往是不够的。常见的做法是将引导程序分为两部分，第一部分位于 MBR 中，负责加载并执行第二部分。第二部分位于硬盘的其他位置，负责为操作系统内核准备环境并最终加载内核。

### 分区表

MBR 分区表有四个分区项，每项 16 字节，格式如下：

| Offset | Size(bytes) | Description                                       |
| ------ | ----------- | ------------------------------------------------- |
| 0x00   | 1           | 0x00 表示不可引导，0x80 表示可引导                |
| 0x01   | 3           | 分区起始 CHS 地址                                 |
| 0x04   | 1           | 分区类型                                          |
| 0x05   | 3           | 分区结束 CHS 地址                                 |
| 0x08   | 4           | 分区起始 LBA 地址                                 |
| 0x0C   | 4           | 分区总扇区数                                      |

分区表项的第一字节用于判断该分区是否可引导。对于一个可引导分区，它的第一个扇区应该存有一段引导程序。这个扇区通常被称为 Volume Boot Record（VBR）。

CHS 地址字段有 3 字节，其组成如下表所示：

| Offset | Size            | Description                        |
| ------ | --------------- | ---------------------------------- |
| 0x01   | 1 byte          | 磁头                               |
| 0x02   | 6 bits          | 扇区（该字节高 2 位是柱面的高 2 位 |
| 0x03   | 1 byte          | 柱面                               |

CHS 字段不是独立的三个字节，而是一个 3 字节的位域结构，CHS 字段的存储不考虑大端序或小端序。其他字段则需要考虑大端序或小端序。

因为分区总扇区数字段只有 4 字节，所以 MBR 支持的最大硬盘容量只有 $2^32\times512$ 字节，也就是 2TB。

## 启动过程

对于 BIOS + MBR 的组合，大致的启动过程如下：

1. BIOS 加电自检后加载 MBR 到内存 `0x7c00` 处，并跳转执行
2. MBR 的引导程序检查分区表，找到可引导分区，将 VBR 加载到内存并跳转执行
3. VBR 的引导程序为内核准备环境并加载执行内核

事实上，VBR 并不是必须的。只要第一部分引导程序可以将第二部分引导程序加载到内存，那么完全可以将第二部分引导程序放在硬盘的任意位置。因此，可引导分区也不是必须的。

## CHS 地址与 LBA 地址

CHS 地址是一种早期的机械硬盘寻址方式，其中 C 代表 Cylinder（柱面），H 代表 Head（磁头），S 代表 Sector（扇区）。

机械硬盘主要由盘片、磁头和电机组成。一个电机带动盘片旋转，一个电机控制磁头移动，磁头悬浮在盘片上方画出的圆形轨道称为「磁道」。不同盘片上的相同磁道共同组成一个「柱面」。一条磁道被等分成多个小块，这些小块称为「扇区」。CHS 地址是一个三维坐标系，垂直坐标是磁头，决定使用哪个盘片，径向坐标是柱面，决定使用哪条磁道，角坐标是扇区，决定使用磁道的哪一部分。

在 IBM-PC 兼容机的 BIOS 中，使用 3 字节来表示 CHS 地址，其中柱面 10 比特，磁头 8 比特，扇区 6 比特，而一个扇区的大小通常是 512 字节。除了扇区从 1 开始计数，柱面和磁头都是从 0 开始计数。据此可以计算 CHS 地址的最大寻址范围：`(512 bytes/sector) x (63 sectors/track) x (256 tracks/cylinder) x 1024 cylinders = 8064MiB`。然而早期的 Microsoft DOS/IBM PC DOS 存在 bug 使得使用有 256 个磁头的设备时会导致系统崩溃，尽管后面该 bug 已经修复，但只使用 255 个磁头已经成为事实标准。因此 CHS 地址的最大寻址范围通常是：`(512 bytes/sector) x (63 sectors/track) x (255 tracks/cylinder) x 1024 cylinders = 8032.5MiB`。

LBA（Linera Block Address），线性块地址，一种用线性整数表示硬盘数据块（扇区）的寻址方法。「线性」意味着可以通过 `LBA + 1` 访问当前扇区的下一扇区。LBA 有 LBA22，LBA28，LBA48 等不同版本，后面的数字代表可用的位数。

CHS 地址转 LBA 地址有公式：$LBA = (C\timesTPC+H)\timesSPT+(S-1)$。

LBA 地址转 CHS 地址有公式：$C = \frac{LBA}{TPC\timesSPT}$，$H = (\frac{LBA}{SPT} \bmod TPC$，$S = (LBA \bmod SPT) + 1$。

## 制作 MBR 镜像

只要第一个扇区的最后两个字节是 `0x55AA` 就会被为 MBR 镜像。

```bash
# boot.asm
# [bits 16]
# times 510 - ($ - $$) db 0
# ; x86 架构是小端序
# dw 0xaa55

nasm -f bin boot.asm boot.bin

file boot.bin
# boot.bin: DOS/MBR boot sector
```

### 创建分区

```bash
# 创建 1MiB 的镜像文件
qemu-img create boot.img 1M

# 写入引导程序
dd bs=512 count=1 conv=notrunc if=boot.bin of=boot.img

# 创建分区
sfdisk boot.img < part.sfdisk

# part.sfdisk 创建分区的模板文件
# label: dos
# label-id: 0x12345678
# unit: sectors
# device: boot.img
# # 起始 LBA 地址为 1，扇区数为 2047，分区类型 0x83
# boot.img1 : start=1, size=2047, type=83

# 查看分区信息
fdisk -l boot.img
# Disk boot.img: 1 MiB, 1048576 bytes, 2048 sectors
# Units: sectors of 1 * 512 = 512 bytes
# Sector size (logical/physical): 512 bytes / 512 bytes
# I/O size (minimum/optimal): 512 bytes / 512 bytes
# Disklabel type: dos
# Disk identifier: 0x12345678
#
# Device     Boot Start   End Sectors    Size Id Type
# boot.img1           1  2047    2047 1023.5K 83 Linux
```

使用 `xxd` 查看镜像的二进制数据。

```
                      分区表第一项开始位置，0x1be
                                             |
                                             v
000001b0: 0000 0000 0000 0000 7856 3412 0000 0000
000001c0: 0200 8320 2000 0100 0000 ff07 0000 0000
```

- 第一个字节是 `0x00` 表示该分区不可引导
- 随后 3 字节是「分区起始 CHS 地址」，`0x000200` 表示 CHS 地址 `0/0/2`，即硬盘第二个扇区
- 随后 1 字节是「分区类型」，`0x83` 代表 Linux
- 紧接着的 3 字节是「分区结束 CHS 地址」，`0x202000`，表示 CHS 地址 `0/32/32`
- 随后 4 字节是「分区起始 LBA 地址」，`0x00000001` 表示硬盘第二个扇区
- 最后 4 字节是「分区总扇区数」，`0x0000ff07` 表示有 2047 个扇区

## 参考资料

1. Wikipedia(2025-05-27). [Master boot record](https://en.wikipedia.org/wiki/Master_boot_record).
2. Wikipedia(2024-11-07). [Volume boot record](https://en.wikipedia.org/wiki/Volume_boot_record).
3. OSDev.Org(2015-11-13). [Volume boot record](https://wiki.osdev.org/Volume_Boot_Record).
4. OSDev.Org(2023-07-09). [MBR(x86)](https://wiki.osdev.org/MBR(x86)).
5. OSDev.Org(2024-01-05). [Partition Table](https://wiki.osdev.org/Partition_Table).
6. OSDev.Org(2025-02-11). [Boot Sequence](https://wiki.osdev.org/Boot_Sequence).
7. Wikipedia(2025-08-06). [Cylinder-head-sector](https://en.wikipedia.org/wiki/Cylinder-head-sector).
8. Wikipedia(2025-05-13). [Logical block addressing](https://en.wikipedia.org/wiki/Logical_block_addressing).
