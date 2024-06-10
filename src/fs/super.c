#include <oak/assert.h>
#include <oak/buffer.h>
#include <oak/debug.h>
#include <oak/device.h>
#include <oak/fs.h>
#include <oak/stat.h>
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

// release super_block_t in super_table
void put_super(super_block_t *sb) {
    if (!sb)
        return;
    assert(sb->count > 0);
    sb->count--;
    if (sb->count)
        return;

    sb->dev = EOF;
    iput(sb->imount);
    iput(sb->iroot);

    for (int i = 0; i < sb->desc->imap_blocks; i++)
        brelse(sb->imaps[i]);
    for (int i = 0; i < sb->desc->zmap_blocks; i++)
        brelse(sb->zmaps[i]);

    brelse(sb->buf);
}

super_block_t *read_super(dev_t dev) {
    // if super_table has
    super_block_t *sb = get_super(dev);
    if (sb) {
        return sb;
        sb->count++;
    }

    DEBUGK("reading super block of device %d\n", dev);

    sb = get_free_super();
    buffer_t *buf = bread(dev, 1);
    sb->buf = buf;
    sb->desc = (super_desc_t *)buf->data;
    sb->dev = dev;
    sb->count = 1;

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

    // 初始化根目录 inode
    root->iroot = iget(device->dev, 1);  // 获得根目录 inode
    root->imount = iget(device->dev, 1); // 根目录挂载 inode

    root->iroot->mount = device->dev;
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

int sys_mount(char *devname, char *dirname, int flags) {
    DEBUGK("mount %s to %s\n", devname, dirname);

    inode_t *devinode = NULL;
    inode_t *dirinode = NULL;
    super_block_t *sb = NULL;
    devinode = namei(devname);
    if (!devinode)
        goto rollback;
    if (!ISBLK(devinode->desc->mode))
        goto rollback;

    dev_t dev = devinode->desc->zone[0];

    dirinode = namei(dirname);
    if (!dirinode)
        goto rollback;

    if (!ISDIR(dirinode->desc->mode))
        goto rollback;

    if (dirinode->count != 1 || dirinode->mount)
        goto rollback;

    sb = read_super(dev);
    if (sb->imount)
        goto rollback;

    sb->iroot = iget(dev, 1);
    sb->imount = dirinode;
    dirinode->mount = dev;
    iput(devinode);
    return 0;

rollback:
    put_super(sb);
    iput(devinode);
    iput(dirinode);
    return EOF;
}

int sys_umount(char *target) {
    DEBUGK("umount %s\n", target);
    inode_t *inode = NULL;
    super_block_t *sb = NULL;
    int ret = EOF;

    inode = namei(target);
    if (!inode)
        goto rollback;

    if (!ISBLK(inode->desc->mode) && inode->nr != 1)
        goto rollback;

    if (inode == root->imount)
        goto rollback;

    dev_t dev = inode->dev;
    if (ISBLK(inode->desc->mode)) {
        dev = inode->desc->zone[0];
    }

    sb = get_super(dev);
    if (!sb->imount)
        goto rollback;

    if (!sb->imount->mount) {
        DEBUGK("warning super block mount = 0\n");
    }

    if (list_size(&sb->inode_list) > 1)
        goto rollback;

    iput(sb->iroot);
    sb->iroot = NULL;

    sb->imount->mount = 0;
    iput(sb->imount);
    sb->imount = NULL;
    ret = 0;

rollback:
    put_super(sb);
    iput(inode);
    return ret;
}
