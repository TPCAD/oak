#include "oak/buffer.h"
#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include "oak/fs/stat.h"
#include "oak/list.h"
#include "oak/vdevice.h"
#include <oak/fs/minix.h>
#include <oak/string.h>

#define SUPER_NR 16

static super_block_t super_table[SUPER_NR]; // 超级块表
static super_block_t *root;                 // 根文件系统

static super_block_t *search_free_super_block() {
    for (size_t i = 0; i < SUPER_NR; i++) {
        super_block_t *sb = &super_table[i];
        if (sb->dev == -1) {
            return sb;
        }
    }
    kpanic("[fs] No more super block...\n");
    return NULL; // no need
}

/**
 *  @brief  从超级块表中搜索指定设备的超级块
 *  @param  dev  设备号
 *  @return  超级块信息
 */
super_block_t *search_super_block(u32 dev) {
    for (size_t i = 0; i < SUPER_NR; i++) {
        super_block_t *sb = &super_table[i];
        if (sb->dev == dev) {
            return sb;
        }
    }
    return NULL;
}

/**
 *  @brief  释放超级块表中的超级块
 *  @param  sb  超级块
 */
void put_super(super_block_t *sb) {
    if (!sb)
        return;
    kassert(sb->count > 0);
    sb->count--;
    if (sb->count)
        return;

    sb->dev = EOF;
    inode_free(sb->imount);
    inode_free(sb->iroot);

    for (int i = 0; i < sb->sblk->imap_blocks; i++)
        buffer_release(sb->imaps[i]);
    for (int i = 0; i < sb->sblk->zmap_blocks; i++)
        buffer_release(sb->zmaps[i]);

    buffer_release(sb->buf);
}

/**
 *  @brief  解析指定设备的超级块
 *  @param  dev  设备号
 *  @return  超级块信息
 */
super_block_t *parse_super_block(u32 dev) {
    super_block_t *sb = search_super_block(dev);
    if (sb) {
        sb->count++;
        return sb;
    }

    sb = search_free_super_block();

    buffer_t *buf = buffer_read(dev, 1);
    sb->buf = buf;
    sb->sblk = (sblk_desc_t *)buf->data;
    sb->dev = dev;
    sb->count = 1;

    kassert(sb->sblk->magic == MINIX1_MAGIC);

    memset(sb->imaps, 0, sizeof(sb->imaps));
    memset(sb->zmaps, 0, sizeof(sb->zmaps));

    // 超级块后就是 inode 位图块，使用一个临时变量获得逻辑块位图块的起始块号
    u32 idx = 2;
    for (size_t i = 0; i < sb->sblk->imap_blocks; i++) {
        kassert(i < IMAP_NR);
        if ((sb->imaps[i] = buffer_read(dev, idx))) {
            idx++;
        } else {
            break;
        }
    }
    for (size_t i = 0; i < sb->sblk->zmap_blocks; i++) {
        kassert(i < ZMAP_NR);
        if ((sb->zmaps[i] = buffer_read(dev, idx))) {
            idx++;
        } else {
            break;
        }
    }

    return sb;
}

/**
 *  @brief  挂载第一个分区的文件系统为根文件系统
 */
static void mount_root() {
    KDEBUG("[fs] Mount root file system...\n");
    vdevice_t *vdev = vdevice_search(VDEV_IDE_PART, 0);
    kassert(vdev);

    root = parse_super_block(vdev->dev);

    root->iroot = inode_search(vdev->dev, 1);
    root->imount = inode_search(vdev->dev, 1);
    root->iroot->mount = vdev->dev;
}

int super_mount(char *devname, char *dirname, int flags) {
    KDEBUG("mount %s to %s\n", devname, dirname);

    inode_t *devinode = NULL;
    inode_t *dirinode = NULL;
    super_block_t *sb = NULL;
    devinode = namei(devname);
    if (!devinode)
        goto rollback;
    if (!ISBLK(devinode->inode->mode))
        goto rollback;

    u32 dev = devinode->inode->zone[0];

    dirinode = namei(dirname);
    if (!dirinode)
        goto rollback;

    if (!ISDIR(dirinode->inode->mode))
        goto rollback;

    if (dirinode->count != 1 || dirinode->mount)
        goto rollback;

    sb = parse_super_block(dev);
    if (sb->imount)
        goto rollback;

    sb->iroot = inode_search(dev, 1);
    sb->imount = dirinode;
    dirinode->mount = dev;
    inode_free(devinode);
    return 0;

rollback:
    put_super(sb);
    inode_free(devinode);
    inode_free(dirinode);
    return EOF;
}

int super_umount(char *target) {
    KDEBUG("umount %s\n", target);
    inode_t *inode = NULL;
    super_block_t *sb = NULL;
    int ret = EOF;

    inode = namei(target);
    if (!inode)
        goto rollback;

    if (!ISBLK(inode->inode->mode) && inode->idx != 1)
        goto rollback;

    if (inode == root->imount)
        goto rollback;

    u32 dev = inode->dev;
    if (ISBLK(inode->inode->mode)) {
        dev = inode->inode->zone[0];
    }

    sb = parse_super_block(dev);
    if (!sb->imount)
        goto rollback;

    if (!sb->imount->mount) {
        KDEBUG("warning super block mount = 0\n");
    }

    if (list_size(&sb->inode_list) > 1)
        goto rollback;

    inode_free(sb->iroot);
    sb->iroot = NULL;

    sb->imount->mount = 0;
    inode_free(sb->imount);
    sb->imount = NULL;
    ret = 0;

rollback:
    put_super(sb);
    inode_free(inode);
    return ret;
}

void super_init() {
    for (size_t i = 0; i < SUPER_NR; i++) {
        super_block_t *sb = &super_table[i];
        sb->dev = -1;
        sb->sblk = NULL;
        sb->buf = NULL;
        sb->iroot = NULL;
        sb->imount = NULL;
        list_init(&sb->inode_list);
    }

    mount_root();
}
