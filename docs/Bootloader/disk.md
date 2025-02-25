# Disk

保护模式下无法再使用 BIOS 中断，需要通过 PIO Mode 进行硬盘读取。

## ATA/IDE

ATA（或 Parellel ATA）是由 Western Digital 和 Compaq 开发的一种用于连接存储设备的接口标准。ATA 本身是由 Western Digital 的 IDE 接口发展而来。

## PIO Mode

PIO Mode（Programmed Input/Output Mode）是一种早期 ATA 接口用于在存储设备与计算机之间传输数据的机制，其最大的特点是所有数据都必须经过 CPU，因此会占用大量的 CPU 资源。但是在引导阶段没有其他进程，所以 PIO Mode 是一种适合在引导阶段读取硬盘的方式。

### 主/从总线

现在的硬盘控制芯片几乎都支持两条 ATA 总线。主总线（Primary Bus）默认由 I/O 端口 0x1F0～0x1F7 控制，而从总线（Secondary Bus）默认由 I/O 端口 0x3F0～0x3F7 控制。

### I/O 端口

| 端口（偏移量）| 读/写 | 功能                  | 描述                        | 大小（LBA28）|
| --------------| ----- | --------------------- | --------------------------- | ------------ |
| 0             | R/W   | Data Register         | 读/写数据                   | 16 位        |
| 1             | R     | Error Register        | 上一次命令的错误            | 8 位         |
| 1             | W     | Features Register     | 控制命令的特殊接口功能      | 8 位         |
| 2             | R/W   | Sector Count Register | 读/写的扇区数（0 是特殊值） | 8 位         |
| 3             | R/W   | LBA low               | 起始扇区 LBA 地址 0～7 位   | 8 位         |
| 4             | R/W   | LBA mid               | 起始扇区 LBA 地址 8～15 位  | 8 位         |
| 5             | R/W   | LBA high              | 起始扇区 LBA 地址 16～23 位 | 8 位         |
| 6             | R/W   | Drive Register        | 见下方补充                  | 8 位         |
| 7             | R     | Status Register       | 读取当前状态                | 8 位         |
| 7             | W     | Command Register      | 向设备发生 ATA 命令         | 8 位         |

- I/O base + 6：
    - 0～3：起始扇区 LBA 地址 24～27 位
    - 4：0 主驱动器，1 从驱动器
    - 6：0 CHS 模式，1 LBA 模式
    - 5、7：固定为 1
- I/O base + 7，Read：
    - 0xEC：识别硬盘
    - 0x20：读硬盘
    - 0x30：写硬盘
- I/O base + 7，Write：
    - 0：ERR
    - 3：DRQ，数据准备完毕
    - 7：BSY，硬盘繁忙

## 参考资料

[Parellel ATA - Wiki](https://en.wikipedia.org/wiki/Parallel_ATA)
