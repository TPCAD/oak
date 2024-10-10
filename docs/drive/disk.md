# 硬盘驱动

## IDE 协议

IDE（Integrated Drive Electronics，集成电子驱动），也被成为 ATA，一种早期的硬盘传输协议。

### 主从通道

一个硬盘控制器上通常有一条主通道（primary bus），一条从通道（secondary bus）。通常情况下，主通道由 IO 端口 `0x1F0~0x1F7` 控制，其设备控制寄存器为 `0x3F6`，从通道由 `0x170~0x177` 控制，其设备控制寄存器为 `0x376`。每个通道可以挂载两个硬盘，一个主盘，一个从盘。

### 寻址模式

CHS 寻址模式通过柱面、磁头、扇区三个参数进行寻址。LBA 模式则将硬盘抽象称连续的块，只需要一个 LBA 地址即可确定一块存储区域。LBA28 以 28 bits 表示 LBA 地址，最大可以表示 128 GB。

### 寄存器

控制器有许多端口用于控制其行为或反映其状态，有些寄存器的读写操作拥有不同的含义。

#### 硬盘控制端口

| Primary 通道 | Secondary 通道 | in 操作      | out 操作     |
| ------------ | -------------- | ------------ | ------------ |
| 0x1F0        | 0x170          | Data         | Data         |
| 0x1F1        | 0x171          | Error        | Features     |
| 0x1F2        | 0x172          | Sector count | Sector count |
| 0x1F3        | 0x173          | LBA low      | LBA low      |
| 0x1F4        | 0x174          | LBA mid      | LBA mid      |
| 0x1F5        | 0x175          | LBA high     | LBA high     |
| 0x1F6        | 0x176          | Device       | Device       |
| 0x1F7        | 0x177          | Status       | Command      |

- 0x1F0：16 bit 端口，用于读写数据
- 0x1F1（in）：检测前一个指令的错误
- 0x1F2：读写扇区的数量
- 0x1F3：起始扇区的 0～7 位
- 0x1F4：起始扇区的 8～15 位
- 0x1F5：起始扇区的 16～23 位
- 0x1F6：
  - 0～3：起始扇区的 24～27 位
  - 4： 0 主盘, 1 从盘
  - 6： 寻址方式，0 CHS, 1 LBA
  - 5～7：固定为 1
- 0x1F7： out
  - 0xEC： 识别硬盘
  - 0x20： 读硬盘
  - 0x30： 写硬盘
- 0x1F7： in / 8 bit
  - 0 ERR
  - 3 DRQ 数据准备完毕
  - 7 BSY 硬盘繁忙
  
#### 错误寄存器（0x1F1）

| 位  | 缩写  | 描述                      |
| --- | ----- | ------------------------- |
| 0   | AMNF  | Address mark not found.   |
| 1   | TKZNF | Track zero not found.     |
| 2   | ABRT  | Aborted command.          |
| 3   | MCR   | Media change request.     |
| 4   | IDNF  | ID not found.             |
| 5   | MC    | Media changed.            |
| 6   | UNC   | Uncorrectable data error. |
| 7   | BBK   | Bad Block detected.       |

#### 状态寄存器（0x1F7）

| 位  | 缩写 | 描述                                             |
| --- | ---- | ------------------------------------------------ |
| 0   | ERR  | 标志错误发生                                     |
| 1   | IDX  | 索引，总为 0                                     |
| 2   | CORR | 纠正数据，总为 0                                 |
| 3   | DRQ  | PIO 数据准备完毕                                 |
| 4   | SRV  | 重叠模式服务请求                                 |
| 5   | DF   | 驱动器故障错误                                   |
| 6   | RDY  | 当驱动器停机或发生错误后，位是清除的。否则置位。 |
| 7   | BSY  | 硬盘繁忙                                         |

#### 备用状态寄存器（0x3F6）

读取设备控制寄存器总会得到备用状态寄存器的值，而备用状态寄存器的值与状态寄存器的值相同，不同的是，读取备用状态寄存器不会影响中断。

#### 设备控制寄存器（0x3F6）

## IDE 设备驱动

### 通道初始化

```c
typedef struct ide_ctrl_t {
    char name[8];                  // 控制器名称
    lock_t lock;                   // 互斥锁
    u16 iobase;                    // 通道基地址
    ide_disk_t disks[IDE_DISK_NR]; // 挂载硬盘数组
    ide_disk_t *active;            // 当前使用硬盘
    u8 control;                    // 设备控制寄存器值
    struct task_t *waiter;         // 等待该控制器的进程列表
} ide_ctrl_t;
```

### 识别硬盘

```c
/**
 *  @brief  识别硬盘
 *  @param  disk  硬盘
 *  @param  buf  缓冲区
 *  @return  错误码
 *
 *  主要获取硬盘的 LBA 总块数
 */
static u32 ide_identify(ide_disk_t *disk, u16 *buf)
```

识别过程需加锁。

### 硬盘初始化

```c
typedef struct ide_disk_t {
    char name[8];            // 硬盘名称
    struct ide_ctrl_t *ctrl; // 控制器指针
    u8 selector;             // 0x1F6 端口的值
    bool master;             // 主盘/从盘
    u32 total_lba;           // 总 LBA 块数
    u32 cylinders;           // 不使用
    u32 heads;               // 不使用
    u32 sectors;             // 不使用
    ide_part_t parts[IDE_PART_NR]; // 分区数组
} ide_disk_t;
```

### 读写硬盘
