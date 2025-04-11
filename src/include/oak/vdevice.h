#ifndef OAK_DEVICE_H
#define OAK_DEVICE_H

#include <oak/types.h>

#define NAMELEN 16

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
};

typedef struct vdevice_t {
    char name[NAMELEN]; // 设备名
    int type;           // 设备类型
    int subtype;        // 设备子类型
    u32 dev;            // 设备号
    u32 parent;         // 父设备号
    void *ptr;          // 设备指针
    // 控制设备
    int (*ioctl)(void *dev, int cmd, void *args, int flags);
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

#endif
