#include <oak/assert.h>
#include <oak/device.h>
#include <oak/fs.h>
#include <oak/stat.h>
#include <oak/stdio.h>
#include <oak/syscall.h>

extern file_t file_table[];

void dev_init() {
    mkdir("/dev", 0755);

    device_t *device = NULL;

    // 第一个虚拟磁盘作为 /dev 文件系统
    device = device_find(DEV_RAMDISK, 0);
    assert(device);
    devmkfs(device->dev, 0);

    super_block_t *sb = read_super(device->dev);
    sb->iroot = iget(device->dev, 1);
    sb->imount = namei("/dev");
    sb->imount->mount = device->dev;

    device = device_find(DEV_CONSOLE, 0);
    mknod("/dev/console", IFCHR | 0200, device->dev);

    device = device_find(DEV_KEYBOARD, 0);
    mknod("/dev/keyboard", IFCHR | 0400, device->dev);

    char name[32];

    // disk
    for (size_t i = 0; true; i++) {
        device = device_find(DEV_IDE_DISK, i);
        if (!device)
            break;
        sprintf(name, "/dev/%s", device->name);
        mknod(name, IFBLK | 0600, device->dev);
    }

    // partition
    for (size_t i = 0; true; i++) {
        device = device_find(DEV_IDE_PART, i);
        if (!device) {
            break;
        }
        sprintf(name, "/dev/%s", device->name);
        mknod(name, IFBLK | 0600, device->dev);
    }

    // memory disk
    for (size_t i = 1; true; i++) {
        device = device_find(DEV_RAMDISK, i);
        if (!device) {
            break;
        }
        sprintf(name, "/dev/%s", device->name);
        mknod(name, IFBLK | 0600, device->dev);
    }

    // serial device
    for (size_t i = 0; true; i++) {
        device = device_find(DEV_SERIAL, i);
        if (!device) {
            break;
        }
        sprintf(name, "/dev/%s", device->name);
        mknod(name, IFCHR | 0600, device->dev);
    }

    link("/dev/console", "/dev/stdout");
    link("/dev/console", "/dev/stderr");
    link("/dev/keyboard", "/dev/stdin");

    file_t *file;
    inode_t *inode;
    file = &file_table[STDIN_FILENO];
    inode = namei("/dev/stdin");
    file->inode = inode;
    file->mode = inode->desc->mode;
    file->flags = O_RDONLY;
    file->offset = 0;

    file = &file_table[STDOUT_FILENO];
    inode = namei("/dev/stdout");
    file->inode = inode;
    file->mode = inode->desc->mode;
    file->flags = O_WRONLY;
    file->offset = 0;

    file = &file_table[STDERR_FILENO];
    inode = namei("/dev/stderr");
    file->inode = inode;
    file->mode = inode->desc->mode;
    file->flags = O_WRONLY;
    file->offset = 0;
}
