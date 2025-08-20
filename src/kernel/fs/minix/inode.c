#include "oak/bitmap.h"
#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include "oak/fs/stat.h"
#include "oak/list.h"
#include "oak/task.h"
#include "oak/types.h"
#include <oak/buffer.h>
#include <oak/fs/minix.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/syscall.h>

#define INODE_NR 64
// 系统当前读取到内存的所有 inode（来自不同磁盘）
static inode_t inode_table[INODE_NR];

/**
 *  @brief  从 inode 表中寻找空闲 inode
 *  @return  空闲 inode
 */
static inode_t *search_free_inode() {
    for (size_t i = 0; i < INODE_NR; i++) {
        inode_t *inode = &inode_table[i];
        if (inode->dev == -1) {
            return inode;
        }
    }
    kpanic("[fs] No more inode...\n");
    return NULL;
}

static void free_inode(inode_t *inode) {
    kassert(inode != inode_table);
    kassert(inode->count == 0);
    inode->dev = -1;
}

inode_t *inode_get_root_inode() { return (inode_t *)inode_table; }

/**
 *  @brief  计算 inode 所在磁盘块号
 *  @param  sb  超级块
 *  @param  nr  inode 号
 *  @return  磁盘块号
 */
static inline u32 calc_inode_block(super_block_t *sb, u32 nr) {
    return 2 + sb->sblk->imap_blocks + sb->sblk->zmap_blocks +
           (nr - 1) / BLOCK_INODES;
}

/**
 *  @brief  在超级块的 inode 链表中寻找指定 inode
 *  @param  dev  设备号
 *  @param  nr  inode 号
 *  @return  inode 指针
 */
static inode_t *search_inode(u32 dev, u32 nr) {
    super_block_t *sb = super_search_by_devnum(dev);
    kassert(sb);
    list_t *list = &sb->inode_list;

    for (list_node_t *node = list->head.next; node != &list->tail;
         node = node->next) {
        inode_t *inode = element_entry(inode_t, node, node);
        if (inode->idx == nr) {
            return inode;
        }
    }
    return NULL;
}

/**
 *  @brief  检查 inode 是否有挂载目录
 *  @param  inode
 *  @return  若有挂载目录则返回挂载的超级块
 */
static inode_t *fit_inode(inode_t *inode) {
    if (!inode->mount)
        return inode;

    super_block_t *sb = super_search_by_devnum(inode->mount);
    kassert(sb);
    inode_free(inode);
    inode = sb->iroot;
    inode->count++;
    return inode;
}

inode_t *build_inode(u32 dev, u32 nr) {
    task_t *curr_task = task_current_running();
    inode_t *inode = inode_search(dev, nr);
    kassert(inode->inode->nlinks == 0);

    inode->buf->dirty = true;

    inode->inode->mode = 0777 & (~curr_task->umask);
    inode->inode->uid = curr_task->uid;
    inode->inode->size = 0;
    inode->inode->mtime = inode->atime = time();
    inode->inode->gid = curr_task->gid;
    inode->inode->nlinks = 1;

    return inode;
}

/**
 *  @brief  寻找指定 inode
 *  @param  dev  设备号
 *  @param  nr  inode 号
 *  @return  inode 指针
 *
 *  优先从超级块的 inode 链表中寻找，若没有则读取硬盘。
 */
inode_t *inode_search(u32 dev, u32 nr) {
    inode_t *inode = search_inode(dev, nr);
    if (inode) {
        inode->count++;
        inode->atime = time();
        return fit_inode(inode);
    }

    super_block_t *sb = super_search_by_devnum(dev);
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

    inode->inode =
        &((inode_desc_t *)buf->data)[(inode->idx - 1) % BLOCK_INODES];
    inode->ctime = inode->inode->mtime;
    inode->atime = time();

    return inode;
}

/**
 *  @brief  释放内存中的 inode
 *  @param  inode  inode 指针
 */
void inode_free(inode_t *inode) {
    if (!inode) {
        return;
    }

    if (inode->buf->dirty) {
        buffer_write(inode->buf);
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
 *  @brief  读取 inode 对应的数据到缓存
 *  @param  inode  要读取的 inode
 *  @param  buf  缓存
 *  @param  len  要读取的字节数
 *  @param  offset  读取起始位置
 *  @return  读取的字节数
 */
int inode_read(inode_t *inode, char *buf, u32 len, i32 offset) {
    // 文件或目录
    kassert(ISFILE(inode->inode->mode) || ISDIR(inode->inode->mode));

    // 偏移量大于文件大小
    if (offset >= inode->inode->size) {
        return EOF;
    }

    u32 begin = offset;
    u32 left = MIN(len, inode->inode->size - offset);
    while (left) {
        u32 nr = inode_calc_block(inode, offset / BLOCK_SIZE, false);
        kassert(nr);
        // 将文件块读入缓存
        buffer_t *data_buf = buffer_read(inode->dev, nr);
        // 文件块中的偏移量
        u32 start = offset % BLOCK_SIZE;
        // 在当前文件块中读取的字符数
        u32 chars = MIN(BLOCK_SIZE - start, left);
        // 更新偏移量和剩余字符数
        offset += chars;
        left -= chars;
        // 文件块中的偏移指针
        char *ptr = data_buf->data + start;
        memcpy(buf, ptr, chars);
        // 更新缓存位置
        buf += chars;
        // 释放文件块
        buffer_release(data_buf);
    }

    inode->atime = time();
    return offset - begin;
}

/**
 *  @brief  写入缓存数据到对应的 inode
 *  @param  inode  要写入的 inode
 *  @param  buf  缓存
 *  @param  len  要写入的字节数
 *  @param  offset  写入起始位置
 *  @return  写入的字节数
 *
 *  不允许写目录
 */
int inode_write(inode_t *inode, char *buf, u32 len, i32 offset) {
    // 文件
    kassert(ISFILE(inode->inode->mode));

    u32 begin = offset;
    u32 left = len;

    while (left) {
        // 不存在则创建
        u32 nr = inode_calc_block(inode, offset / BLOCK_SIZE, true);

        // 将文件块读入缓存
        buffer_t *data_buf = buffer_read(inode->dev, nr);
        data_buf->dirty = true;
        // 文件块中的偏移量
        u32 start = offset % BLOCK_SIZE;
        // 文件块中的偏移指针
        char *ptr = data_buf->data + start;
        // 在当前文件块中读取的字符数
        u32 chars = MIN(BLOCK_SIZE - start, left);
        // 更新偏移量和剩余字符数
        offset += chars;
        left -= chars;

        if (offset > inode->inode->size) {
            inode->inode->size = offset;
            inode->buf->dirty = true;
        }

        memcpy(ptr, buf, chars);

        buf += chars;

        buffer_release(data_buf);
    }

    inode->inode->mtime = time();
    inode->atime = inode->inode->mtime;

    buffer_write(inode->buf);
    return offset - begin;
}

/**
 *  @brief  计算 inode 第 zone_idx 块的磁盘块号
 *  @param  inode  inode 指针
 *  @param  zone_idx  inode 的 zone 索引
 *  @param  create  是否创建该块
 *  @return  磁盘块号
 */
u32 inode_calc_block(inode_t *inode, u32 zone_idx, bool create) {
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
 *  @brief  将 inode 的所有数据块对应的位图位置 0
 *  @param  inode  inode 指针
 *  @param  array  zone 数组
 *  @param  index  zone 数组索引
 *  @param  level
 */
static void inode_blk_free(inode_t *inode, u16 *array, int index, int level) {
    if (!array[index]) {
        return;
    }

    if (!level) {
        block_free_bit(inode->dev, array[index]);
        return;
    }

    buffer_t *buf = buffer_read(inode->dev, array[index]);
    for (size_t i = 0; i < BLOCK_INDEXES; i++) {
        inode_blk_free(inode, (u16 *)buf->data, i, level - 1);
    }
    buffer_release(buf);
    block_free_bit(inode->dev, array[index]);
}

/**
 *  @brief  释放 inode 的所有数据块
 *  @param  inode  inode 指针
 */
void inode_truncate(inode_t *inode) {
    if (!ISFILE(inode->inode->mode) && !ISDIR(inode->inode->mode)) {
        return;
    }

    // 释放直接块
    for (size_t i = 0; i < DIREC_BLOCKS; i++) {
        inode_blk_free(inode, inode->inode->zone, i, 0);
        inode->inode->zone[i] = 0;
    }

    // 释放一级间接块
    inode_blk_free(inode, inode->inode->zone, DIREC_BLOCKS, 1);
    inode->inode->zone[DIREC_BLOCKS] = 0;

    // 释放二级间接块
    inode_blk_free(inode, inode->inode->zone, DIREC_BLOCKS + 1, 2);
    inode->inode->zone[DIREC_BLOCKS + 1] = 0;

    inode->inode->size = 0;
    inode->buf->dirty = true;
    inode->inode->mtime = time();
    buffer_write(inode->buf);
}

/**
 *  @brief  从逻辑块位图分配一位
 *  @param  dev  设备号
 *  @return  磁盘块索引
 */
u32 block_alloc_bit(u32 dev) {
    super_block_t *sb = super_search_by_devnum(dev);
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
    super_block_t *sb = super_search_by_devnum(dev);
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
    super_block_t *sb = super_search_by_devnum(dev);
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
    super_block_t *sb = super_search_by_devnum(dev);
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
        inode_t *inode = &inode_table[i];
        inode->dev = -1;
    }
}
