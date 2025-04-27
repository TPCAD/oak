#include "oak/syscall.h"
#include "oak/vdevice.h"
#include <oak/debug/kassert.h>
#include <oak/fs/minix.h>
#include <oak/stdio.h>
#include <oak/task.h>
#include <oak/types.h>

#define FILE_NR 128

// 系统当前打开的所有文件，前三个文件是标准输入，标准输出，标准错误
file_t file_table[FILE_NR];

/**
 *  @brief  从 file_table 获取一个空闲 file_t
 *  @return  file_t 指针
 */
file_t *file_search_table() {
    for (size_t i = 3; i < FILE_NR; i++) {
        file_t *file = &file_table[i];
        if (!file->count) {
            file->count++;
            return file;
        }
    }
    kpanic("[fs] Exceed max open files...");
    return NULL; // no need
}

/**
 *  @brief  释放一个 file_t
 *  @param  file  要释放的 file_t
 */
void file_free_table(file_t *file) {
    kassert(file->count > 0);
    file->count--;
    if (!file->count) {
        inode_free(file->inode);
    }
}

/**
 *  @brief  系统调用打开文件
 *  @param  filename  文件路径
 *  @param  flags  打开标志
 *  @param  mode  文件属性
 *  @return  文件号
 */
fd_t file_open(char *filename, int flags, int mode) {
    inode_t *inode = inode_open(filename, flags, mode);
    if (!inode)
        return EOF;

    task_t *curr_task = task_current_running();
    fd_t fd = task_find_fd(curr_task);
    file_t *file = file_search_table();
    kassert(curr_task->files[fd] == NULL);
    curr_task->files[fd] = file;

    file->inode = inode;
    file->flags = flags;
    file->count = 1;
    file->mode = inode->inode->mode;
    file->offset = 0;

    if (flags & O_APPEND) {
        file->offset = file->inode->inode->size;
    }
    return fd;
}

/**
 *  @brief  系统调用创建并打开
 *  @param  filename  文件路径
 *  @param  mode  文件属性
 *  @return  return
 */
int file_create(char *filename, int mode) {
    return file_open(filename, O_CREAT | O_TRUNC, mode);
}

/**
 *  @brief  系统调用关闭文件
 *  @param  fd  文件号
 *  @return  return
 */
void file_close(fd_t fd) {
    kassert(fd < TASK_FILE_NR);
    task_t *curr_task = task_current_running();
    file_t *file = curr_task->files[fd];
    if (!file)
        return;

    kassert(file->inode);
    file_free_table(file);
    task_free_fd(curr_task, fd);
}

/**
 *  @brief  系统调用读文件
 *  @param  fd  文件号
 *  @param  buf  缓冲区
 *  @param  count  读的字节数
 *  @return  return
 */
int file_read(fd_t fd, char *buf, int count) {
    // 文件必须已打开
    task_t *curr_task = task_current_running();
    file_t *file = curr_task->files[fd];
    kassert(file);
    kassert(count > 0);
    int len = 0;

    // 无写权限
    if ((file->flags & O_ACCMODE) == O_WRONLY)
        return EOF;

    inode_t *inode = file->inode;
    if (ISCHR(inode->inode->mode)) { // 字符设备
        kassert(inode->inode->zone[0]);
        len = vdevice_read(inode->inode->zone[0], buf, count, 0, 0);
        return len;
    } else if (ISBLK(inode->inode->mode)) { // 块设备
        kassert(inode->inode->zone[0]);
        // vdevice_t *device = vdevice_get(inode->inode->zone[0]);
        kassert(file->offset % BLOCK_SIZE == 0);
        kassert(count % BLOCK_SIZE == 0);
        len = vdevice_read(inode->inode->zone[0], buf, count / BLOCK_SIZE,
                           file->offset / BLOCK_SIZE, 0);
    } else { // 普通文件
        len = inode_read(inode, buf, count, file->offset);
    }
    // 更新文件偏移值
    if (len != EOF) {
        file->offset += len;
    }
    return len;
}

/**
 *  @brief  系统调用写文件
 *  @param  fd  文件号
 *  @param  buf  缓冲区
 *  @param  count  写的字节数
 *  @return  return
 */
int file_write(unsigned int fd, char *buf, int count) {
    task_t *curr_task = task_current_running();
    file_t *file = curr_task->files[fd];
    kassert(file);
    kassert(count > 0);

    // 文件只读
    if ((file->flags & O_ACCMODE) == O_RDONLY)
        return EOF;

    inode_t *inode = file->inode;
    kassert(inode);
    int len = 0;
    if (ISCHR(inode->inode->mode)) {
        kassert(inode->inode->zone[0]);
        // vdevice_t *vdevice = vdevice_get(inode->inode->zone[0]);
        len = vdevice_write(inode->inode->zone[0], buf, count, 0, 0);
        return len;
    } else if (ISBLK(inode->inode->mode)) {
        kassert(inode->inode->zone[0]);
        // vdevice_t *vdevice = vdevice_get(inode->inode->zone[0]);
        kassert(file->offset % BLOCK_SIZE == 0);
        kassert(count % BLOCK_SIZE == 0);
        len = vdevice_write(inode->inode->zone[0], buf, count / BLOCK_SIZE,
                            file->offset / BLOCK_SIZE, 0);
        return len;
    } else {
        len = inode_write(inode, buf, count, file->offset);
    }
    // 更新文件偏移值
    if (len != EOF) {
        file->offset += len;
    }

    return len;
}

/**
 *  @brief  设置文件偏移位置
 *  @param  fd  文件号
 *  @param  offset  偏移量
 *  @param  whence  偏移起始位置
 *  @return  文件偏移位置
 */
int file_lseek(fd_t fd, i32 offset, whence_t whence) {
    kassert(fd < TASK_FILE_NR);

    task_t *curr_task = task_current_running();
    file_t *file = curr_task->files[fd];

    kassert(file);
    kassert(file->inode);

    switch (whence) {
    case SEEK_SET:
        // 直接设置偏移
        kassert(offset >= 0);
        file->offset = offset;
        break;
    case SEEK_CUR:
        // 从当前位置开始偏移
        kassert(file->offset + offset >= 0);
        file->offset += offset;
        break;
    case SEEK_END:
        // 从结束位置开始偏移
        kassert(file->inode->inode->size + offset >= 0);
        file->offset = file->inode->inode->size + offset;
        break;
    default:
        kpanic("[fs] whence not defined !!!");
        break;
    }
    return file->offset;
}

/**
 *  @brief  读取目录
 *  @param  param  desc
 *  @return  return
 */
int file_readdir(fd_t fd, dentry_t *dir, u32 count) {
    return file_read(fd, (char *)dir, sizeof(dentry_t));
}

void devfile_init() {
    mkdir("/dev", 0755);

    vdevice_t *device = NULL;

    device = vdevice_search(VDEV_CONSOLE, 0);
    mknod("/dev/console", IFCHR | 0200, device->dev);

    device = vdevice_search(VDEV_KEYBOARD, 0);
    mknod("/dev/keyboard", IFCHR | 0400, device->dev);

    char name[32];

    for (size_t i = 0; true; i++) {
        device = vdevice_search(VDEV_IDE_DISK, i);
        if (!device)
            break;
        sprintf(name, "/dev/%s", device->name);
        mknod(name, IFBLK | 0600, device->dev);
    }

    for (size_t i = 0; true; i++) {
        device = vdevice_search(VDEV_IDE_PART, i);
        if (!device) {
            break;
        }
        sprintf(name, "/dev/%s", device->name);
        mknod(name, IFBLK | 0600, device->dev);
    }

    for (size_t i = 0; true; i++) {
        device = vdevice_search(VDEV_SERIAL, i);
        if (!device) {
            break;
        }
        sprintf(name, "/dev/%s", device->name);
        mknod(name, IFCHR | 0600, device->dev);
    }

    // 创建标准输入输出
    link("/dev/console", "/dev/stdout");
    link("/dev/console", "/dev/stderr");
    link("/dev/keyboard", "/dev/stdin");

    file_t *file;
    inode_t *inode;
    file = &file_table[STDIN_FILENO];
    inode = namei("/dev/stdin");
    file->inode = inode;
    file->mode = inode->inode->mode;
    file->flags = O_RDONLY;
    file->offset = 0;

    file = &file_table[STDOUT_FILENO];
    inode = namei("/dev/stdout");
    file->inode = inode;
    file->mode = inode->inode->mode;
    file->flags = O_WRONLY;
    file->offset = 0;

    file = &file_table[STDERR_FILENO];
    inode = namei("/dev/stderr");
    file->inode = inode;
    file->mode = inode->inode->mode;
    file->flags = O_WRONLY;
    file->offset = 0;
}

void file_init() {
    // 跳过标准输入输出
    for (size_t i = 3; i < FILE_NR; i++) {
        file_t *file = &file_table[i];
        file->mode = 0;
        file->count = 0;
        file->flags = 0;
        file->offset = 0;
        file->inode = NULL;
    }
}
