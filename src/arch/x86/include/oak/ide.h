#ifndef OAK_IDE_H
#define OAK_IDE_H

#include <oak/mutex.h>
#include <oak/types.h>

#define SECTOR_SIZE 512 // 扇区大小

#define IDE_CTRL_NR 2 // 控制器数量
#define IDE_DISK_NR 2 // 每个控制器可挂载磁盘数量

// IDE 磁盘
typedef struct ide_disk_t {
    char name[8];            // 磁盘名称
    struct ide_ctrl_t *ctrl; // 控制器指针
    u8 selector;             // 磁盘选择
    bool master;             // 主盘
    u32 total_lba;           // 可以扇区数
    u32 cylinders;
    u32 heads;
    u32 sectors;
} ide_disk_t;

// IDE 控制器
typedef struct ide_ctrl_t {
    char name[8];                  // 控制器名称
    lock_t lock;                   // 控制器锁
    u16 iobase;                    // IO 寄存器基址
    ide_disk_t disks[IDE_DISK_NR]; // 磁盘
    ide_disk_t *selected_disk;     // 当前选择的磁盘
    u8 control;                    // 控制字节
    struct task_t *waiter;         // 等待控制器的任务
} ide_ctrl_t;

int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, u32 lba);
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, u32 lba);

#endif
