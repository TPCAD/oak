#include "oak/bitmap.h"
#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include "oak/list.h"
#include <oak/buffer.h>
#include <oak/fs/minix.h>
#include <oak/syscall.h>

#define INODE_NR 64
// 系统当前读取到内存的所有 inode（来自不同磁盘）
static inode_info_t inode_table[INODE_NR];

/**
 *  @brief  从 inode 表中寻找空闲 inode
 *  @return  空闲 inode
 */
static inode_info_t *search_free_inode() {
    for (size_t i = 0; i < INODE_NR; i++) {
        inode_info_t *inode = &inode_table[i];
        if (inode->dev == -1) {
            return inode;
        }
    }
    kpanic("[fs] No more inode...\n");
    return NULL;
}

static void free_inode(inode_info_t *inode) {
    kassert(inode != inode_table);
    kassert(inode->count == 0);
    inode->dev = -1;
}

inode_info_t *get_root_inode() { return inode_table; }

/**
 *  @brief  计算 inode 所在磁盘块号
 *  @param  sb  超级块
 *  @param  nr  inode 号
 *  @return  磁盘块号
 */
static inline u32 calc_inode_block(sblk_info_t *sb, u32 nr) {
    return 2 + sb->sblk->imap_blocks + sb->sblk->zmap_blocks +
           (nr - 1) / BLOCK_INODES;
}

/**
 *  @brief  在超级块的 inode 链表中寻找指定 inode
 *  @param  dev  设备号
 *  @param  nr  inode 号
 *  @return  inode 指针
 */
static inode_info_t *search_inode(u32 dev, u32 nr) {
    sblk_info_t *sb = search_super_block(dev);
    kassert(sb);
    list_t *list = &sb->inode_list;

    for (list_node_t *node = list->head.next; node != &list->tail;
         node = node->next) {
        inode_info_t *inode = element_entry(inode_info_t, node, node);
        if (inode->idx == nr) {
            return inode;
        }
    }
    return NULL;
}

/**
 *  @brief  寻找指定 inode
 *  @param  dev  设备号
 *  @param  nr  inode 号
 *  @return  inode 指针
 *
 *  优先从超级块的 inode 链表中寻找，若没有则读取硬盘。
 */
inode_info_t *inode_search(u32 dev, u32 nr) {
    inode_info_t *inode = search_inode(dev, nr);
    if (inode) {
        inode->count++;
        inode->atime = time();
        return inode;
    }

    sblk_info_t *sb = search_super_block(dev);
    kassert(sb);

    kassert(nr <= sb->sblk->inodes);

    inode = search_free_inode();
    inode->dev = dev;
    inode->idx = nr;
    inode->count = 1;
    list_push(&sb->inode_list, &inode->node);

    u32 block = calc_inode_block(sb, inode->idx);
    buffer_t *buf = buffer_read(inode->dev, block);
    inode->buf = buf;

    inode->inode = &((inode_t *)buf->data)[(inode->idx - 1) % BLOCK_INODES];
    inode->ctime = inode->inode->mtime;
    inode->atime = time();

    return inode;
}

/**
 *  @brief  释放内存中的 inode
 *  @param  inode  inode 指针
 */
void inode_free(inode_info_t *inode) {
    if (!inode) {
        return;
    }

    inode->count--;
    if (inode->count) {
        return;
    }

    buffer_release(inode->buf);

    list_remove(&inode->node);

    free_inode(inode);
}

/**
 *  @brief  计算 inode 第 zone_idx 块的磁盘块号
 *  @param  inode  inode 指针
 *  @param  zone_idx  inode 的 zone 索引
 *  @param  create  是否创建该块
 *  @return  磁盘块号
 */
u32 inode_calc_block(inode_info_t *inode, u32 zone_idx, bool create) {
    kassert(zone_idx >= 0 && zone_idx < TOTAL_BLOCKS);

    u16 index = zone_idx;
    u16 *arr = inode->inode->zone;

    buffer_t *buf = inode->buf;

    // 配合 reckon 释放 buffer
    buf->count++;

    int level = 0;   // 当前处理等级
    int divider = 1; // 间接块数量

    // 直接块
    if (zone_idx < DIREC_BLOCKS) {
        goto reckon;
    }

    zone_idx -= DIREC_BLOCKS;

    // 一级间接块
    if (zone_idx < INDIRECT1_BLOCKS) {
        index = DIREC_BLOCKS;
        level = 1;
        divider = 1;
        goto reckon;
    }

    // 二级间接块
    zone_idx -= INDIRECT1_BLOCKS;
    kassert(zone_idx < INDIRECT2_BLOCKS);
    index = DIREC_BLOCKS + 1;
    level = 2;
    divider = BLOCK_INDEXES;

reckon:
    for (; level >= 0; level--) {
        /* 当 zone_idx 是直接块时，index 就是对应的直接块索引。
         *
         * 当 zone_idx 是一级间接块时，会进行两次循环。第一次的 index 是一级间接
         * 块，若不存在则分配逻辑块（或直接返回）。
         * 第二次的 index 则是一级间接块索引。
         *
         * 当 zone_idx 是一级间接块时，会进行三次循环。第一次的 index 是二级间接
         * 块，若不存在则分配逻辑块（或直接返回）。
         */
        if (!arr[index] && create) {
            arr[index] = block_alloc_bit(inode->dev);
            buf->dirty = true;
        }
        /* 直接块时释放 inode 对应 buffer（减少引用技术）。
         *
         * 一级间接块时，第一次释放 inode 对应 buffer，第二次释放一级间接块
         * （zone[7]）。
         *
         * 二级间接块时，第一次释放 inode 对应 buffer，第二次释放二级间接块
         * （zone[8]）。
         * */
        buffer_release(buf);

        // level 为 0 或索引不存在，直接返回
        if (!level || !arr[index]) {
            return arr[index];
        }

        /* 一级间接块时读 zone[7]
         *
         * 二级间接块时，第一次读 zone[8]
         * */
        buf = buffer_read(inode->dev, arr[index]);
        /* 一级间接块时为一级间接块索引
         *
         * 二级间接块时，第一次为一级间接块索引
         * */
        index = zone_idx / divider;
        // 二级间接块时，
        zone_idx = zone_idx % divider;
        divider /= BLOCK_INDEXES;
        arr = (u16 *)buf->data;
    }
}

/**
 *  @brief  从逻辑块位图分配一位
 *  @param  dev  设备号
 *  @return  磁盘块索引
 */
u32 block_alloc_bit(u32 dev) {
    sblk_info_t *sb = search_super_block(dev);
    kassert(sb);

    buffer_t *buf = NULL;
    u32 bit = -1;
    bitmap_t map;

    for (size_t i = 0; i < sb->sblk->zmap_blocks; i++) {
        buf = sb->zmaps[i];
        kassert(buf);

        bitmap_create(&map, (u8 *)buf->data, BLOCK_SIZE, i * BLOCK_BITS, false);
        bit = bitmap_find_bits(&map, 1);
        if (bit != -1) {
            kassert(bit < sb->sblk->zones && bit != 0);
            buf->dirty = true;
            break;
        }
    }
    buffer_write(buf);
    KDEBUG("block bit alloc %d\n", bit);
    return bit + sb->sblk->firstdatazone - 1;
}

/**
 *  @brief  释放逻辑块位图指定位
 *  @param  dev  设备号
 *  @param  idx  磁盘索引
 */
void block_free_bit(u32 dev, u32 idx) {
    sblk_info_t *sb = search_super_block(dev);
    kassert(sb);
    kassert(idx >= sb->sblk->firstdatazone);
    idx -= sb->sblk->firstdatazone - 1;
    kassert(idx < sb->sblk->zones);

    buffer_t *buf = NULL;
    bitmap_t map;

    for (size_t i = 0; i < sb->sblk->zmap_blocks; i++) {
        if (idx > BLOCK_BITS * (i + 1)) {
            continue;
        }

        buf = sb->zmaps[i];
        kassert(buf);

        bitmap_create(&map, (u8 *)buf->data, BLOCK_SIZE, i * BLOCK_BITS, false);
        kassert(bitmap_is_set(&map, idx));
        bitmap_set_bit(&map, idx, 0);
        buf->dirty = true;
        break;
    }

    buffer_write(buf);
    KDEBUG("block bit free %d\n", idx);
}

/**
 *  @brief  从 inode 位图分配一位
 *  @param  dev  设备号
 *  @return  位图索引
 */
u32 inode_alloc_bit(u32 dev) {
    sblk_info_t *sb = search_super_block(dev);
    kassert(sb);

    buffer_t *buf = NULL;
    u32 bit = -1;
    bitmap_t map;

    for (size_t i = 0; i < sb->sblk->imap_blocks; i++) {
        buf = sb->imaps[i];
        kassert(buf);

        bitmap_create(&map, (u8 *)buf->data, BLOCK_SIZE, i * BLOCK_BITS, false);
        bit = bitmap_find_bits(&map, 1);
        if (bit != -1) {
            kassert(bit < sb->sblk->inodes && bit != 0);
            buf->dirty = true;
            break;
        }
    }
    buffer_write(buf);
    KDEBUG("inode bit alloc %d\n", bit);
    return bit;
}

/**
 *  @brief  释放 inode 位图指定位
 *  @param  dev  设备号
 *  @param  idx  位图索引
 */
void inode_free_bit(u32 dev, u32 idx) {
    sblk_info_t *sb = search_super_block(dev);
    kassert(sb);
    kassert(idx < sb->sblk->inodes);

    buffer_t *buf = NULL;
    bitmap_t map;

    for (size_t i = 0; i < sb->sblk->imap_blocks; i++) {
        if (idx > BLOCK_BITS * (i + 1)) {
            continue;
        }

        buf = sb->imaps[i];
        kassert(buf);

        bitmap_create(&map, (u8 *)buf->data, BLOCK_SIZE, i * BLOCK_BITS, false);
        kassert(bitmap_is_set(&map, idx));
        bitmap_set_bit(&map, idx, 0);
        buf->dirty = true;
        break;
    }

    buffer_write(buf);
    KDEBUG("inode bit free %d\n", idx);
}

void inode_init() {
    for (size_t i = 0; i < INODE_NR; i++) {
        inode_info_t *inode = &inode_table[i];
        inode->dev = -1;
    }
}
