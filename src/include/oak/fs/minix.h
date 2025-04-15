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

#define BLOCK_INODES (BLOCK_SIZE / sizeof(inode_desc_t)) // 块内 inode 数量
#define BLOCK_DENTRIES (BLOCK_SIZE / sizeof(dentry_t))   // 块内 dentry 数量
// 间接块对应的数据块是一个索引数组，一个数据块可以有 512 个索引
#define BLOCK_INDEXES (BLOCK_SIZE / sizeof(u16)) // 块内索引数

#define DIREC_BLOCKS 7 // 直接块数量
#define INDIRECT1_BLOCKS BLOCK_INDEXES
#define INDIRECT2_BLOCKS (INDIRECT1_BLOCKS * INDIRECT1_BLOCKS)
#define TOTAL_BLOCKS (DIREC_BLOCKS + INDIRECT1_BLOCKS + INDIRECT2_BLOCKS)

#define SEPARATOR1 '/'  // 目录分隔符 1
#define SEPARATOR2 '\\' // 目录分隔符 2
#define IS_SEPARATOR(c)                                                        \
    ((c) == SEPARATOR1 || (c) == SEPARATOR2) // 字符是否位目录分隔符

typedef struct inode_desc_t {
    u16 mode;    // 文件类型和属性（rwx）
    u16 uid;     // 用户 id（文件拥有者标识符）
    u32 size;    // 文件大小（字节数）
    u32 mtime;   // 修改时间戳（UTC 时间）
    u8 gid;      // 组 id(文件拥有者所在的组)
    u8 nlinks;   // 链接数（多少个文件目录项指向该 inode）
    u16 zone[9]; // 直接（0-6）、间接（7）或双重间接（8）逻辑块号
} inode_desc_t;

typedef struct inode_t {
    inode_desc_t *inode;
    struct buffer_t *buf;
    u32 dev;
    u32 idx;      // inode 号
    u32 count;    // 引用计数
    time_t atime; // 访问时间
    time_t ctime; // 修改时间
    list_node_t node;
    u32 mount;
} inode_t;

typedef struct sblk_desc_t {
    u16 inodes;        // 节点数
    u16 zones;         // 逻辑块数
    u16 imap_blocks;   // inode 位图所占用的数据块数
    u16 zmap_blocks;   // 逻辑块位图所占用的数据块数
    u16 firstdatazone; // 第一个数据逻辑块号
    u16 log_zone_size; // log2(每逻辑块数据块数)
    u32 max_size;      // 文件最大长度
    u16 magic;         // 文件系统魔数
} sblk_desc_t;

typedef struct super_block_t {
    sblk_desc_t *sblk;
    struct buffer_t *buf;
    struct buffer_t *imaps[IMAP_NR];
    struct buffer_t *zmaps[ZMAP_NR];
    int dev;
    list_t inode_list; // 使用中的 inode
    inode_t *iroot;    // 根目录 inode
    inode_t *imount;
} super_block_t;

// 文件目录项结构
typedef struct dentry_t {
    u16 nr;              // inode 索引
    char name[NAME_LEN]; // 文件名
} dentry_t;

enum file_flag {
    O_RDONLY = 00,      // 只读方式
    O_WRONLY = 01,      // 只写方式
    O_RDWR = 02,        // 读写方式
    O_ACCMODE = 03,     // 文件访问模式屏蔽码
    O_CREAT = 00100,    // 如果文件不存在就创建
    O_EXCL = 00200,     // 独占使用文件标志
    O_NOCTTY = 00400,   // 不分配控制终端
    O_TRUNC = 01000,    // 若文件已存在且是写操作，则长度截为 0
    O_APPEND = 02000,   // 以添加方式打开，文件指针置为文件尾
    O_NONBLOCK = 04000, // 非阻塞方式打开和操作文件
};

typedef struct file_t {
    inode_t *inode; // 文件 inode
    u32 count;      // 引用计数
    i32 offset;     // 文件偏移
    int flags;      // 文件标记
    int mode;       // 文件模式
} file_t;

typedef enum whence_t {
    SEEK_SET = 1, // 直接设置偏移
    SEEK_CUR,     // 当前位置偏移
    SEEK_END      // 结束位置偏移
} whence_t;

super_block_t *search_super_block(u32 dev);

u32 inode_alloc_bit(u32 dev);
void inode_free_bit(u32 dev, u32 idx);

u32 block_alloc_bit(u32 dev);
void block_free_bit(u32 dev, u32 idx);

inode_t *inode_search(u32 dev, u32 nr);
void inode_free(inode_t *inode);

u32 inode_calc_block(inode_t *inode, u32 zone_idx, bool create);

inode_t *inode_get_root_inode();

inode_t *namei(char *pathname);
inode_t *named(char *pathname, char **next);

inode_t *build_inode(u32 dev, u32 nr);
int inode_read(inode_t *inode, char *buf, u32 len, i32 offset);
int inode_write(inode_t *inode, char *buf, u32 len, i32 offset);
void inode_truncate(inode_t *inode);
inode_t *inode_open(char *pathname, int flag, int mode);

void file_free_table(file_t *file);
file_t *file_search_table();

#endif // !OAK_MINIX_H
