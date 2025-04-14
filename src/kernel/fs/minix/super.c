#include "oak/buffer.h"
#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
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
 *  @brief  解析指定设备的超级块
 *  @param  dev  设备号
 *  @return  超级块信息
 */
super_block_t *parse_super_block(u32 dev) {
    super_block_t *sb = search_super_block(dev);
    if (sb) {
        return sb;
    }

    sb = search_free_super_block();

    buffer_t *buf = buffer_read(dev, 1);
    sb->buf = buf;
    sb->sblk = (sblk_desc_t *)buf->data;
    sb->dev = dev;

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
