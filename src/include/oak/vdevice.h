#ifndef OAK_DEVICE_H
#define OAK_DEVICE_H

#include <oak/list.h>
#include <oak/task.h>
#include <oak/types.h>

#define NAMELEN 16
#define REQ_READ 0  // 块设备读请求
#define REQ_WRITE 1 // 块设备写请求

typedef struct block_request_t {
    u32 dev;          // 设备号
    u32 type;         // 请求类型
    u32 idx;          // 扇区位置
    u32 count;        // 扇区数量
    int flags;        // 特殊标志
    u8 *buf;          // 缓冲区
    task_t *task;     // 请求任务
    list_node_t node; // 链表结点
} block_request_t;

// 设备类型
enum vdevice_type_t {
    VDEV_NULL,  // 空设备
    VDEV_CHAR,  // 字符设备
    VDEV_BLOCK, // 块设备
};

// 设备子类型
enum vdevice_subtype_t {
    VDEV_CONSOLE = 1, // 控制台
    VDEV_KEYBOARD,    // 键盘
    VDEV_SERIAL,      // 串口
    VDEV_IDE_DISK,    // IDE 磁盘
    VDEV_IDE_PART,    // IDE 分区
};

// 设备控制命令
enum vdevice_cmd_t {
    VDEV_CMD_SECTOR_START = 1,
    VDEV_CMD_SECTOR_COUNT,
};

typedef struct vdevice_t {
    char name[NAMELEN];                                      // 设备名
    int type;                                                // 设备类型
    int subtype;                                             // 设备子类型
    u32 dev;                                                 // 设备号
    u32 parent;                                              // 父设备号
    void *ptr;                                               // 设备指针
    list_t request_list;                                     // 块设备请求链表
    int (*ioctl)(void *dev, int cmd, void *args, int flags); // 控制设备
    // 读设备
    int (*read)(void *dev, void *buf, size_t count, u32 idx, int flags);
    // 写设备
    int (*write)(void *dev, void *buf, size_t count, u32 idx, int flags);
} vdevice_t;

u32 vdevice_install(int type, int subtype, void *ptr, char *name, u32 parent,
                    void *ioctl, void *read, void *write);
vdevice_t *vdevice_search(int type, u32 idx);
vdevice_t *vdevice_get(u32 nr);
int vdevice_ioctl(u32 dev, int cmd, void *args, int flags);
int vdevice_read(u32 dev, void *buf, size_t count, u32 idx, int flags);
int vdevice_write(u32 dev, void *buf, size_t count, u32 idx, int flags);
void vdevice_request(u32 dev, void *buf, size_t count, u32 idx, int flags,
                     u32 type);

#endif
