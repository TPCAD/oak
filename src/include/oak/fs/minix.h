#ifndef OAK_MINIX_H
#define OAK_MINIX_H

#include "oak/list.h"
#include <oak/types.h>

#define BLOCK_SIZE 1024 // 块大小
#define SECTOR_SIZE 512 // 扇区大小

#define MINIX1_MAGIC 0x137F // 文件系统魔数
#define NAME_LEN 14         // 文件名长度

#define IMAP_NR 8 // inode 块位图块数最大值
#define ZMAP_NR 8 // 逻辑块位图块数最大值

#define BLOCK_BITS (BLOCK_SIZE * 8) // 块位图比特数

typedef struct inode_t {
    u16 mode;    // 文件类型和属性（rwx）
    u16 uid;     // 用户 id（文件拥有者标识符）
    u32 size;    // 文件大小（字节数）
    u32 mtime;   // 修改时间戳（UTC 时间）
    u8 gid;      // 组 id(文件拥有者所在的组)
    u8 nlinks;   // 链接数（多少个文件目录项指向该 inode）
    u16 zone[9]; // 直接（0-6）、间接（7）或双重间接（8）逻辑块号
} inode_t;

typedef struct super_block_t {
    u16 inodes;        // 节点数
    u16 zones;         // 逻辑块数
    u16 imap_blocks;   // inode 位图所占用的数据块数
    u16 zmap_blocks;   // 逻辑块位图所占用的数据块数
    u16 firstdatazone; // 第一个数据逻辑块号
    u16 log_zone_size; // log2(每逻辑块数据块数)
    u32 max_size;      // 文件最大长度
    u16 magic;         // 文件系统魔数
} super_block_t;

typedef struct super_block_info_t {
    super_block_t *sblk;
    struct buffer_t *buf;
    struct buffer_t *imaps[IMAP_NR];
    struct buffer_t *zmaps[ZMAP_NR];
    int dev;
    list_t inode_list; // 使用中的 inode
    inode_t *iroot;    // 根目录 inode
    inode_t *imount;
} sblk_info_t;

// 文件目录项结构
typedef struct dentry_t {
    u16 nr;              // inode 索引
    char name[NAME_LEN]; // 文件名
} dentry_t;

sblk_info_t *search_super_block(u32 dev);

u32 inode_alloc_bit(u32 dev);
void inode_free_bit(u32 dev, u32 idx);

u32 block_alloc_bit(u32 dev);
void block_free_bit(u32 dev, u32 idx);

#endif // !OAK_MINIX_H
