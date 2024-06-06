#include <oak/assert.h>
#include <oak/buffer.h>
#include <oak/debug.h>
#include <oak/device.h>
#include <oak/fs.h>
#include <oak/string.h>
#include <oak/types.h>

#define SUPER_NR 16

static super_block_t super_table[SUPER_NR];
static super_block_t *root;

static super_block_t *get_free_super() {
    for (size_t i = 0; i < SUPER_NR; i++) {
        super_block_t *sb = &super_table[i];
        if (sb->dev == EOF) {
            return sb;
        }
    }
    panic("no more super block");
    return NULL; // no use
}

super_block_t *get_super(dev_t dev) {
    for (size_t i = 0; i < SUPER_NR; i++) {
        super_block_t *sb = &super_table[i];
        if (sb->dev == dev) {
            return sb;
        }
    }
    return NULL;
}

super_block_t *read_super(dev_t dev) {
    super_block_t *sb = get_super(dev);
    if (sb) {
        return sb;
    }

    DEBUGK("reading super block of device %d\n", dev);

    sb = get_free_super();
    buffer_t *buf = bread(dev, 1);
    sb->buf = buf;
    sb->desc = (super_desc_t *)buf->data;
    sb->dev = dev;

    assert(sb->desc->magic == MINIX1_MAGIC);

    memset(sb->imaps, 0, sizeof(sb->imaps));
    memset(sb->zmaps, 0, sizeof(sb->zmaps));

    int idx = 2;

    // read inode bitmap
    for (int i = 0; i < sb->desc->imap_blocks; i++) {
        assert(i < IMAP_NR);
        if ((sb->imaps[i] = bread(dev, idx))) {
            idx++;
        } else {
            break;
        }
    }

    // read inode bitmap
    for (int i = 0; i < sb->desc->zmap_blocks; i++) {
        assert(i < ZMAP_NR);
        if ((sb->zmaps[i] = bread(dev, idx))) {
            idx++;
        } else {
            break;
        }
    }
    return sb;
}

static void mount_root() {
    DEBUGK("mount root file system\n");
    device_t *device = device_find(DEV_IDE_PART, 0);
    assert(device);

    root = read_super(device->dev);

    device = device_find(DEV_IDE_PART, 1);
    assert(device);
    super_block_t *sb = read_super(device->dev);

    idx_t idx = ialloc(sb->dev);
    ifree(sb->dev, idx);

    idx = balloc(sb->dev);
    bfree(sb->dev, idx);
}

void super_init() {
    for (size_t i = 0; i < SUPER_NR; i++) {
        super_block_t *sb = &super_table[i];
        sb->dev = EOF;
        sb->desc = NULL;
        sb->buf = NULL;
        sb->iroot = NULL;
        sb->imount = NULL;
        list_init(&sb->inode_list);
    }
    mount_root();
}
