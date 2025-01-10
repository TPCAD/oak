#include <oak/assert.h>
#include <oak/buffer.h>
#include <oak/debug.h>
#include <oak/device.h>
#include <oak/fs.h>
#include <oak/stat.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/task.h>
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

// 从设备读取超级块
super_block_t *read_super(dev_t dev) {
    // if super_table has
    super_block_t *sb = get_super(dev);
    if (sb) {
        sb->count++;
        return sb;
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

    // read zone bitmap
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

// 挂载 root
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

/**
 *  @brief  系统调用 mount，挂载设备到目录
 *  @param  devname  设备名称
 *  @param  dirname  目录名称
 *  @param  flags  标记
 *  @return  错误编码
 */
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

int devmkfs(dev_t dev, u32 icount) {
    super_block_t *sb = NULL;
    buffer_t *buf = NULL;
    int ret = EOF;

    int total_block =
        device_ioctl(dev, DEV_CMD_SECTOR_COUNT, NULL, 0) / BLOCK_SECS;
    assert(total_block);
    assert(icount < total_block);
    if (!icount) {
        icount = total_block / 3;
    }

    sb = get_free_super();
    sb->dev = dev;
    sb->count = 1;

    buf = bread(dev, 1);
    sb->buf = buf;
    buf->dirty = true;

    // 初始化超级块
    super_desc_t *desc = (super_desc_t *)buf->data;
    sb->desc = desc;

    int inode_blocks = div_round_up(icount * sizeof(inode_desc_t), BLOCK_SIZE);
    desc->inodes = icount;
    desc->zones = total_block;
    desc->imap_blocks = div_round_up(icount, BLOCK_BITS);

    int zcount = total_block - desc->imap_blocks - inode_blocks - 2;
    desc->zmap_blocks = div_round_up(zcount, BLOCK_BITS);

    desc->firstdatazone =
        2 + desc->imap_blocks + desc->zmap_blocks + inode_blocks;
    desc->log_zone_size = 0;
    desc->max_size = BLOCK_SIZE * TOTAL_BLOCK;
    desc->magic = MINIX1_MAGIC;

    // 清空位图
    memset(sb->imaps, 0, sizeof(sb->imaps));
    memset(sb->zmaps, 0, sizeof(sb->zmaps));

    int idx = 2;
    for (int i = 0; i < sb->desc->imap_blocks; i++)
        if ((sb->imaps[i] = bread(dev, idx))) {
            memset(sb->imaps[i]->data, 0, BLOCK_SIZE);
            sb->imaps[i]->dirty = true;
            idx++;
        } else
            break;
    for (int i = 0; i < sb->desc->zmap_blocks; i++)
        if ((sb->zmaps[i] = bread(dev, idx))) {
            memset(sb->zmaps[i]->data, 0, BLOCK_SIZE);
            sb->zmaps[i]->dirty = true;
            idx++;
        } else
            break;

    // 初始化位图
    idx = balloc(dev);

    idx = ialloc(dev);
    idx = ialloc(dev);

    // 位图尾部置位
    int counts[] = {
        icount + 1,
        zcount,
    };

    buffer_t *maps[] = {
        sb->imaps[sb->desc->imap_blocks - 1],
        sb->zmaps[sb->desc->zmap_blocks - 1],
    };
    for (size_t i = 0; i < 2; i++) {
        int count = counts[i];
        buffer_t *map = maps[i];
        map->dirty = true;
        int offset = count % (BLOCK_BITS);
        int begin = (offset / 8);
        char *ptr = (char *)map->data + begin;
        memset(ptr + 1, 0xFF, BLOCK_SIZE - begin - 1);
        int bits = 0x80;
        char data = 0;
        int remain = 8 - offset % 8;
        while (remain--) {
            data |= bits;
            bits >>= 1;
        }
        ptr[0] = data;
    }

    // 创建根目录
    task_t *task = running_task();

    inode_t *iroot = new_inode(dev, 1);
    sb->iroot = iroot;

    iroot->desc->mode = (0777 & ~task->umask) | IFDIR;
    iroot->desc->size = sizeof(dentry_t) * 2; // 当前目录和父目录两个目录项
    iroot->desc->nlinks = 2;                  // 一个是 '.' 一个是 name

    buf = bread(dev, bmap(iroot, 0, true));
    buf->dirty = true;

    dentry_t *entry = (dentry_t *)buf->data;
    memset(entry, 0, BLOCK_SIZE);

    strcpy(entry->name, ".");
    entry->nr = iroot->nr;

    entry++;
    strcpy(entry->name, "..");
    entry->nr = iroot->nr;

    brelse(buf);
    ret = 0;
rollback:
    put_super(sb);
    return ret;
}

int sys_mkfs(char *devname, int icount) {
    inode_t *inode = NULL;
    int ret = EOF;

    inode = namei(devname);
    if (!inode)
        goto rollback;

    if (!ISBLK(inode->desc->mode))
        goto rollback;

    dev_t dev = inode->desc->zone[0];
    assert(dev);

    ret = devmkfs(dev, icount);

rollback:
    iput(inode);
    return ret;
}
