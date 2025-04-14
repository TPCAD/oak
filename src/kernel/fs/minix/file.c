#include "oak/vdevice.h"
#include <oak/debug/kassert.h>
#include <oak/fs/minix.h>
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
    if (fd == stdin) {
        vdevice_t *device = vdevice_search(VDEV_KEYBOARD, 0);
        return vdevice_read(device->dev, buf, count, 0, 0);
    }

    // 文件必须已打开
    task_t *curr_task = task_current_running();
    file_t *file = curr_task->files[fd];
    kassert(file);
    kassert(count > 0);

    // 无写权限
    if ((file->flags & O_ACCMODE) == O_WRONLY)
        return EOF;

    inode_t *inode = file->inode;
    int len = inode_read(inode, buf, count, file->offset);
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
    if (fd == stdout || fd == stderr) {
        vdevice_t *device = vdevice_search(VDEV_CONSOLE, 0);
        return vdevice_write(device->dev, buf, count, 0, 0);
    }

    task_t *curr_task = task_current_running();
    file_t *file = curr_task->files[fd];
    kassert(file);
    kassert(count > 0);

    // 文件只读
    if ((file->flags & O_ACCMODE) == O_RDONLY)
        return EOF;

    inode_t *inode = file->inode;
    int len = inode_write(inode, buf, count, file->offset);
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

void file_init() {
    for (size_t i = 0; i < FILE_NR; i++) {
        file_t *file = &file_table[i];
        file->mode = 0;
        file->count = 0;
        file->flags = 0;
        file->offset = 0;
        file->inode = NULL;
    }
}
