#ifndef OAK_DEVICE_H
#define OAK_DEVICE_H

#include <oak/list.h>
#include <oak/types.h>

#define NAMELEN 16

enum device_type_t {
    DEV_NULL,
    DEV_CHAR,
    DEV_BLOCK,
};

enum device_subtype_t {
    DEV_CONSOLE = 1,
    DEV_KEYBOARD,
    DEV_IDE_DISK,
    DEV_IDE_PART,
};

enum device_cmd_t {
    DEV_CMD_SECTOR_START = 1, // get start sector lba
    DEV_CMD_SECTOR_COUNT,     // get sector amount
};

#define REQ_READ 0  // block device read
#define REQ_WRITE 1 // block device write

#define DIRECT_UP 0
#define DIRECT_DOWN 1

typedef struct request_t {
    dev_t dev;           // device number
    u32 type;            // request type
    u32 idx;             // sector position
    u32 count;           // sector amount
    int flags;           // special flags
    u8 *buf;             // buffer
    struct task_t *task; // requested task
    list_node_t node;    // list node
} request_t;

typedef struct device_t {
    char name[NAMELEN];  // device name
    int type;            // device type
    int subtype;         // device sub type
    dev_t dev;           // device number
    dev_t parent;        // father device number
    void *ptr;           // device pointer
    list_t request_list; // block device request list
    bool direct;         // seek direction
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

// block device request
void device_request(dev_t dev, void *buf, u8 count, idx_t idx, int flags,
                    u32 type);
#endif // !OAK_DEVICE_H
