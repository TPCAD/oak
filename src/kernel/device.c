#include <oak/assert.h>
#include <oak/debug.h>
#include <oak/device.h>
#include <oak/string.h>
#include <oak/types.h>

#define DEVICE_NR 64 // device amount

static device_t devices[DEVICE_NR];

static device_t *get_null_device() {
    for (size_t i = 1; i < DEVICE_NR; i++) {
        device_t *device = &devices[i];
        if (device->type == DEV_NULL) {
            return device;
        }
    }
    panic("no more devices\n");
}

int device_ioctl(dev_t dev, int cmd, void *args, int flags) {
    device_t *device = device_get(dev);
    if (device->ioctl) {
        return device->ioctl(device->ptr, cmd, args, flags);
    }
    DEBUGK("ioctl of device %d not implemented!\n", dev);
    return EOF;
}

int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags) {
    device_t *device = device_get(dev);
    if (device->read) {
        return device->read(device->ptr, buf, count, idx, flags);
    }
    DEBUGK("read of device %d not implemented!\n", dev);
    return EOF;
}

int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags) {
    device_t *device = device_get(dev);
    if (device->write) {
        return device->write(device->ptr, buf, count, idx, flags);
    }
    DEBUGK("write of device %d not implemented!\n", dev);
    return EOF;
}

dev_t device_install(int type, int subtype, void *ptr, char *name, dev_t parent,
                     void *ioctl, void *read, void *write) {
    device_t *device = get_null_device();
    device->ptr = ptr;
    device->parent = parent;
    device->type = type;
    device->subtype = subtype;
    strncpy(device->name, name, NAMELEN);
    device->ioctl = ioctl;
    device->read = read;
    device->write = write;
    return device->dev;
}

void device_init() {
    for (size_t i = 0; i < DEVICE_NR; i++) {
        device_t *device = &devices[i];
        strcpy((char *)device->name, "null");
        device->type = DEV_NULL;
        device->subtype = DEV_NULL;
        device->dev = i;
        device->parent = 0;
        device->ioctl = NULL;
        device->read = NULL;
        device->write = NULL;
    }
}

device_t *device_find(int subtype, idx_t idx) {
    idx_t nr = 0;
    for (size_t i = 0; i < DEVICE_NR; i++) {
        device_t *device = &devices[i];
        if (device->subtype != subtype)
            continue;
        if (nr == idx)
            return device;
        nr++;
    }
    return NULL;
}

device_t *device_get(dev_t dev) {
    assert(dev < DEVICE_NR);
    device_t *device = &devices[dev];
    assert(device->type != DEV_NULL);
    return device;
}
