#include "oak/bitmap.h"
#include "oak/debug/kassert.h"
#include "oak/debug/kdebug.h"
#include <oak/buffer.h>
#include <oak/fs/minix.h>

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
