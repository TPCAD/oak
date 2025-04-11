#include <oak/cpu.h>
#include <oak/debug/kassert.h>
#include <oak/debug/kdebug.h>
#include <oak/ide.h>
#include <oak/interrupt/idt.h>
#include <oak/interrupt/pic.h>
#include <oak/io.h>
#include <oak/mm/pmm.h>
#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/task.h>

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERR 0x0001        // 错误寄存器
#define IDE_FEATURE 0x0001    // 功能寄存器
#define IDE_SECTOR 0x0002     // 扇区数量
#define IDE_LBA_LOW 0x0003    // LBA 低字节
#define IDE_LBA_MID 0x0004    // LBA 中字节
#define IDE_LBA_HIGH 0x0005   // LBA 高字节
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令

#define IDE_CMD_READ 0x20     // 读命令
#define IDE_CMD_WRITE 0x30    // 写命令
#define IDE_CMD_IDENTIFY 0xEC // 识别命令

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA

ide_ctrl_t controllers[IDE_CTRL_NR];

void ide_handler(int vector) {
    pic_send_eoi(vector);

    // master 控制器的中断向量号是 0x2e，slave 控制器则是 0x2f，
    // 硬盘的中断 IRQ 是 0xe
    ide_ctrl_t *ctrl = &controllers[vector - IRQ_HARDDISK - 0x20];

    // 读取常规状态寄存器，表示中断处理结束
    u8 state = inb(ctrl->iobase + IDE_STATUS);

    // 唤醒等待磁盘的任务
    if (ctrl->waiter) {
        task_unblock(ctrl->waiter);
        ctrl->waiter = NULL;
    }
}

/**
 *  @brief  打印 IDE 错误信息
 *  @param  ctrl  IDE 控制器
 */
static u32 ide_error(ide_ctrl_t *ctrl) {
    u8 error = inb(ctrl->iobase + IDE_ERR);
    if (error & IDE_ER_BBK)
        KDEBUG("bad block\n");
    if (error & IDE_ER_UNC)
        KDEBUG("uncorrectable data\n");
    if (error & IDE_ER_MC)
        KDEBUG("media change\n");
    if (error & IDE_ER_IDNF)
        KDEBUG("id not found\n");
    if (error & IDE_ER_MCR)
        KDEBUG("media change requested\n");
    if (error & IDE_ER_ABRT)
        KDEBUG("abort\n");
    if (error & IDE_ER_TK0NF)
        KDEBUG("track 0 not found\n");
    if (error & IDE_ER_AMNF)
        KDEBUG("address mark not found\n");
}

/**
 *  @brief  等待磁盘完成操作
 *  @param  ctrl  控制器指针
 *  @param  mask  掩码
 *  @return  成功则返回 0
 */
static u32 ide_busy_wait(ide_ctrl_t *ctrl, u8 mask) {
    while (true) {
        // 从备用状态寄存器中读状态
        u8 state = inb(ctrl->iobase + IDE_ALT_STATUS);
        if (state & IDE_SR_ERR) // 有错误
        {
            ide_error(ctrl);
        }
        if (state & IDE_SR_BSY) // 驱动器忙
        {
            continue;
        }
        if ((state & mask) == mask) // 等待的状态完成
            return 0;
    }
}

/**
 *  @brief  选择磁盘
 *  @param  disk  磁盘指针
 *
 *  向磁盘选择寄存器写入，并设置控制器的 `selected_disk` 字段
 */
static void ide_select_drive(ide_disk_t *disk) {
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->selected_disk = disk;
}

/**
 *  @brief  选择扇区
 *  @param  disk  要写入的磁盘指针
 *  @param  lba  要写入的 LBA 号
 *  @param  count  要写入的扇区数
 */
static void ide_select_sector(ide_disk_t *disk, u32 lba, u8 count) {
    // 输出功能，可省略
    outb(disk->ctrl->iobase + IDE_FEATURE, 0);

    // 读写扇区数量
    outb(disk->ctrl->iobase + IDE_SECTOR, count);

    // LBA 低字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);
    // LBA 中字节
    outb(disk->ctrl->iobase + IDE_LBA_MID, (lba >> 8) & 0xff);
    // LBA 高字节
    outb(disk->ctrl->iobase + IDE_LBA_HIGH, (lba >> 16) & 0xff);

    // LBA 最高四位 + 磁盘选择
    outb(disk->ctrl->iobase + IDE_HDDEVSEL,
         ((lba >> 24) & 0xf) | disk->selector);
    disk->ctrl->selected_disk = disk;
}

/**
 *  @brief  向磁盘读取一个扇区
 *  @param  disk  要读取的磁盘指针
 *  @param  buf  缓冲区
 */
static void ide_pio_read_sector(ide_disk_t *disk, u16 *buf) {
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++) {
        buf[i] = inw(disk->ctrl->iobase + IDE_DATA);
    }
}

/**
 *  @brief  向磁盘写入一个扇区
 *  @param  disk  要写入的磁盘指针
 *  @param  buf  要写入的数据
 */
static void ide_pio_write_sector(ide_disk_t *disk, u16 *buf) {
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++) {
        outw(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}

/**
 *  @brief  以 PIO 方式读磁盘
 *  @param  disk  要读取的磁盘
 *  @param  buf  要读取的数据
 *  @param  count  要读取的扇区数
 *  @param  lba  要读取的 LBA 号
 *  @return  return
 */
int ide_pio_read(ide_disk_t *disk, void *buf, u8 count, u32 lba) {
    kassert(count > 0);
    kassert(!cpu_get_intr_state());

    ide_ctrl_t *ctrl = disk->ctrl;

    lock_acquire(&ctrl->lock);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送读命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0; i < count; i++) {
        task_t *curr_task = task_current_running();
        if (curr_task->state == TASK_RUNNING) {
            ctrl->waiter = curr_task;
            task_block(curr_task, NULL, TASK_BLOCKED);
        }
        ide_busy_wait(ctrl, IDE_SR_DRQ);
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (u16 *)offset);
    }

    lock_release(&ctrl->lock);
    return 0;
}

/**
 *  @brief  以 PIO 方式写磁盘
 *  @param  disk  要写入的磁盘
 *  @param  buf  要写入的数据
 *  @param  count  要写入的扇区数
 *  @param  lba  要写入的 LBA 号
 *  @return  return
 */
int ide_pio_write(ide_disk_t *disk, void *buf, u8 count, u32 lba) {
    kassert(count > 0);
    // 读写过程不允许中断，因为当向控制器发送读写命令后，可能立刻就触发了中断，
    // 中断处理完后接着任务就会把自己阻塞，此后若没有任务再读写硬盘那么该任务就
    // 会被永远阻塞。
    kassert(!cpu_get_intr_state());

    ide_ctrl_t *ctrl = disk->ctrl;

    // 上锁，防止同时写
    lock_acquire(&ctrl->lock);

    KDEBUG("write lba 0x%x\n", lba);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送写命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0; i < count; i++) {
        u32 offset = ((u32)buf + i * SECTOR_SIZE);
        ide_pio_write_sector(disk, (u16 *)offset);

        task_t *curr_task = task_current_running();
        if (curr_task->state == TASK_RUNNING) {
            ctrl->waiter = curr_task;
            task_block(curr_task, NULL, TASK_BLOCKED);
        }

        // 每写一个扇区等待磁盘准备
        ide_busy_wait(ctrl, IDE_SR_NULL);
    }

    lock_release(&ctrl->lock);
    return 0;
}

/**
 *  @brief  初始化 controllers 数组
 */
static void ide_ctrl_init() {
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++) {
        ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx); // 控制器名，e.g. ide0
        lock_init(&ctrl->lock);             // 控制器锁
        ctrl->selected_disk = NULL;         // 当前选择的磁盘

        // 寄存器基址
        if (cidx) // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        } else // 主通道
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }

        // 初始化挂载的磁盘
        for (size_t didx = 0; didx < IDE_DISK_NR; didx++) {
            ide_disk_t *disk = &ctrl->disks[didx];
            sprintf(disk->name, "hd%c", 'a' + cidx * 2 + didx);
            disk->ctrl = ctrl;
            if (didx) // 从盘
            {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            } else // 主盘
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
        }
    }
}

void ide_init() {
    ide_ctrl_init();

    idt_set_intr_handler(IRQ_HARDDISK, ide_handler);
    idt_set_intr_handler(IRQ_HARDDISK2, ide_handler);
    pic_set_intr_mask(IRQ_HARDDISK, true);
    pic_set_intr_mask(IRQ_HARDDISK2, true);
    pic_set_intr_mask(IRQ_CASCADE, true);
}
