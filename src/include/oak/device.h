#ifndef OAK_DEVICE_H
#define OAK_DEVICE_H

#include "oak/types.h"

#define NAMELEN 16

enum device_type_t {
    DEV_NULL,
    DEV_CHAR,
    DEV_BLOCK,
};

enum device_subtype_t {
    DEV_CONSOLE = 1,
    DEV_KEYBOARD,
};

typedef struct device_t {
    char name[NAMELEN]; // device name
    int type;           // device type
    int subtype;        // device sub type
    dev_t dev;          // device number
    dev_t parent;       // father device number
    void *ptr;          // device pointer
    // device control
    int (*ioctl)(void *dev, int cmd, void *args, int flags);
    // read device
    int (*read)(void *dev, void *buf, size_t count, idx_t idx, int flags);
    // write device
    int (*write)(void *dev, void *buf, size_t count, idx_t idx, int flags);
} device_t;

// install device
dev_t device_install(int type, int subtype, void *ptr, char *name, dev_t parent,
                     void *ioctl, void *read, void *write);

// find device with subtype
device_t *device_find(int subtype, idx_t idx);

// find device with device number
device_t *device_get(dev_t dev);

// control device
int device_ioctl(dev_t dev, int cmd, void *args, int flags);

// read device
int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags);

// write device
int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags);
#endif // !OAK_DEVICE_H
