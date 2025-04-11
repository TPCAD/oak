#include <oak/debug/kassert.h>
#include <oak/debug/kdebug.h>
#include <oak/string.h>
#include <oak/types.h>
#include <oak/vdevice.h>

#define VDEVICE_NR 64 // 最大设备数

static vdevice_t vdevices[VDEVICE_NR]; // 设备数组

/**
 *  @brief  搜索设备数组，获取空设备
 *  @return  空设备指针
 */
static vdevice_t *search_null_vdevice() {
    // 0 号设备固定为空设备
    for (size_t i = 1; i < VDEVICE_NR; i++) {
        vdevice_t *vdev = &vdevices[i];
        if (vdev->type == VDEV_NULL) {
            return vdev;
        }
    }
    kpanic("[kernel] No more virtual devices...\n");
    return NULL; // no need
}

/**
 *  @brief  执行设备控制函数
 *  @param  dev  设备号
 *  @param  cmd  设备控制命令
 *  @param  args  控制命令参数
 *  @param  flags  标志
 *  @return  设备控制函数返回值
 */
int vdevice_ioctl(u32 dev, int cmd, void *args, int flags) {
    vdevice_t *vdev = vdevice_get(dev);
    if (vdev->ioctl) {
        return vdev->ioctl(vdev->ptr, cmd, args, flags);
    }
    KDEBUG("ioctl of virtual device %d is not implemented yet...\n", dev);
    return -1;
}

/**
 *  @brief  执行设备读函数
 *  @param  dev  设备号
 *  @param  buf  缓冲区
 *  @param  count
 *  @param  idx
 *  @param  flags  标志
 *  @return  设备读函数返回值
 */
int vdevice_read(u32 dev, void *buf, size_t count, u32 idx, int flags) {
    vdevice_t *vdev = vdevice_get(dev);
    if (vdev->read) {
        return vdev->read(vdev->ptr, buf, count, idx, flags);
    }
    KDEBUG("read of virtual device %d is not implemented yet...\n", dev);
    return -1;
}

/**
 *  @brief  执行设备写函数
 *  @param  dev  设备号
 *  @param  buf  缓冲区
 *  @param  count
 *  @param  idx
 *  @param  flags  标志
 *  @return  设备读函数返回值
 */
int vdevice_write(u32 dev, void *buf, size_t count, u32 idx, int flags) {
    vdevice_t *vdev = vdevice_get(dev);
    if (vdev->write) {
        return vdev->write(vdev->ptr, buf, count, idx, flags);
    }
    KDEBUG("write of virtual device %d is not implemented yet...\n", dev);
    return -1;
}

/**
 *  @brief  安装设备
 *  @param  type  设备类型
 *  @param  subtype  子设备类型
 *  @param  ptr  设备指针
 *  @param  name  设备名称
 *  @param  parent  父设备号
 *  @param  ioctl  设备控制函数
 *  @param  read  设备读函数
 *  @param  write  设备写函数
 *  @return  return
 */
u32 vdevice_install(int type, int subtype, void *ptr, char *name, u32 parent,
                    void *ioctl, void *read, void *write) {
    vdevice_t *vdev = search_null_vdevice();
    vdev->type = type;
    vdev->subtype = subtype;
    vdev->ptr = ptr;
    strncpy(vdev->name, name, NAMELEN);
    vdev->ioctl = ioctl;
    vdev->read = read;
    vdev->write = write;
    return vdev->dev;
}

/**
 *  @brief  根据子设备类型寻找设备
 *  @param  subtype  子设备类型
 *  @param  idx  子设备序号
 *  @return  设备指针
 *
 *  @a idx 是同子设备类型的索引。
 */
vdevice_t *vdevice_search(int subtype, u32 idx) {
    u32 nr = 0;
    for (size_t i = 0; i < VDEVICE_NR; i++) {
        vdevice_t *vdev = &vdevices[i];
        if (vdev->subtype != subtype) {
            continue;
        }
        if (nr == idx) {
            return vdev;
        }
        nr++;
    }
    return NULL;
}

/**
 *  @brief  获取指定设备
 *  @param  nr  设备号
 *  @return  设备指针
 */
vdevice_t *vdevice_get(u32 nr) {
    kassert(nr < VDEVICE_NR);
    vdevice_t *vdev = &vdevices[nr];
    kassert(vdev->type != VDEV_NULL);
    return vdev;
}

/**
 *  @brief  初始化设备数组
 *
 *  所有设备初始化为空设备
 */
void vdevice_init() {
    for (size_t i = 0; i < VDEVICE_NR; i++) {
        vdevice_t *vdev = &vdevices[i];
        strcpy(vdev->name, "null");
        vdev->type = VDEV_NULL;
        vdev->subtype = VDEV_NULL;
        vdev->dev = i;
        vdev->parent = 0;
        vdev->ptr = NULL;
        vdev->ioctl = NULL;
        vdev->read = NULL;
        vdev->write = NULL;
    }
}
