+++
title = 'GUID Partition Table'
date = 2025-08-22T08:28:44+08:00
tags = ['OSDev', 'GPT', 'Partition scheme', 'UEFI']
+++

GPT（GUID Partition Table），全局唯一标识分区表，是一种新的硬盘分区方案，MBR 的替代者，也是 UEFI 标准的一部分。

一块使用 GPT 分区的硬盘其第一个逻辑块必须是「保护性 MBR」，第二个逻辑块必须是「GPT 头部」，之后则是「分区数组」（不一定是的第三个逻辑块），分区数组之后是「可用逻辑块」，这里是硬盘真正存储数据的地方。最后一个可用逻辑块之后就是「备份分区数组」，硬盘的最后一个逻辑块必须是「备份 GPT 头部」。

```
                            First usable blcok      Last usable blcok
                                       |               |
    LBA 0        LBA 1                 v               v                       LBA n
+------------+----------+-------------+-----------------+------------------+------------+
| Protective | GPT      | Partition   |     Usable      | Backup Partition | Backup GPT |
| MBR        | Header   | Entry Array |     Block       | Entry Array      | Header     |
+------------+----------+-------------+-----------------+------------------+------------+
```

## 保护性 MBR

GPT 规定硬盘的第一个逻辑块必须是「保护性 MBR」以保证兼容性。其结构如下：

| Offset | Size(bytes) | Description                                  |
| ------ | ----------- | -------------------------------------------- |
| 0x000  | 440         | MBR 引导程序，不使用                         |
| 0x1B8  | 4           | 不使用，置 0                                 |
| 0x1BC  | 2           | 不使用，置 0                                 |
| 0x1BE  | 16          | 分区表。只使用一项，其余置 0。具体内容见下表 |
| 0x1FE  | 2           | 0x55AA                                       |

分区表项结构如下表所示：

| Offset | Size(bytes) | Description                                                 |
| ------ | ----------- | ----------------------------------------------------------- |
| 0x00   | 1           | 置 0x00 表示不可引导                                        |
| 0x01   | 3           | 分区起始 CHS 地址。置 0x000200（硬盘第二个逻辑块）          |
| 0x04   | 1           | 分区类型。置 0xEE 表示 GPT 保护分区                         |
| 0x05   | 3           | 分区结束 CHS 地址。若无法表示则置 0xFFFFFF                  |
| 0x08   | 4           | 分区起始 LBA 地址。置 0x00000001（硬盘第二个逻辑块）        |
| 0x0C   | 4           | 分区总扇区数。硬盘总逻辑块数减 1，若无法表示则置 0xFFFFFFFF |

## GPT 头部

GPT 头部存储了硬盘分区的各种信息，其结构如下表所示。GPT 硬盘包括一个「主 GPT 头部」，一个「备份 GPT 头部」，主 GPT 头部必须位于硬盘的第二个逻辑块（即 LBA 1），而备份 GPT 头部则必须位于硬盘的最后一个逻辑块。

| Mnemonic                 | Offset | Size(bytes)    | Description                                             |
| ------------------------ | ------ | -------------- | ------------------------------------------------------- |
| Signature                | 0      | 8              | GPT 头签名，0x5452415020494645，「EFI PART」            |
| Revision                 | 8      | 4              | GPT 头版本号，0x00010000                                |
| HeaderSize               | 12     | 4              | GPT 头大小，必须大于或等于 92 且小于或等于逻辑块大小    |
| HeaderCRC32              | 16     | 4              | GPT 头检验和，计算时该字段置 0                          |
| Reserved                 | 20     | 4              | 置 0                                                    |
| MyLBA                    | 24     | 8              | GPT 头所在 LBA                                          |
| AlternateLBA             | 32     | 8              | 备份 GPT 头所在 LBA                                     |
| FirstUsableLBA           | 40     | 8              | 第一个可用 LBA（除去保护性 MBR 和 GPT 头后的第一块）    |
| LastUsableLBA            | 48     | 8              | 最后一个可用 LBA                                        |
| DiskGUID                 | 56     | 16             | 标识当前硬盘的 GUID                                     |
| PartitionEntryLBA        | 72     | 8              | 分区数组所在 LBA                                        |
| NumberOfPartitionEntries | 80     | 4              | 分区数组元素个数                                        |
| SizeOfPartitionEntry     | 84     | 4              | 每个分区数组元素的大小，必须为 128 的偶数倍，单位为字节 |
| PartitionEntryArrayCRC32 | 88     | 4              | 分区数组校验和                                          |
| Reserved                 | 92     | BlockSize - 92 | 逻辑块剩余空间，置 0                                    |

## 分区数组

| Mnemonic                 | Offset | Size(bytes)                | Description                                             |
| ------------------------ | ------ | -------------------------- | ------------------------------------------------------- |
| PartitionTypeGUID        | 0      | 16                         | 标识分区类型的 GUID。置 0 表示该分区项未使用            |
| UniquePartitionGUID      | 16     | 16                         | 标识分区的 GUID                                         |
| StartingLBA              | 32     | 8                          | 分区起始 LBA                                            |
| EndingLBA                | 40     | 8                          | 分区结束 LBA                                            |
| Attributes               | 48     | 8                          | 分区属性                                                |
| PartitionName            | 56     | 72                         | 空终止符字符串                                          |
| Reserved                 | 128    | SizeOfPartitionEntry - 128 | 置 0                                                    |

PartitionTypeGUID 字段与 MBR 的 OSType 字段，用于表示分区的用途，常见分区的 GUID 可以在[这里](https://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_type_GUIDs)找到。

## 参考资料

1. UEFI Specification(2024). [GUID Partition Table(GPT) Disk Layout](https://uefi.org/specs/UEFI/2.11/05_GUID_Partition_Table_Format.html).
2. Wikipedia(2025-08-12). [GUID Partition Table](https://en.wikipedia.org/wiki/GUID_Partition_Table)
