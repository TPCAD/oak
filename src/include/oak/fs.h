#ifndef OAK_FS_H
#define OAK_FS_H

#include <oak/buffer.h>
#include <oak/device.h>
#include <oak/list.h>
#include <oak/types.h>

#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512

#define MINIX1_MAGIC 0x137f
#define NAME_LEN 14

#define IMAP_NR 8
#define ZMAP_NR 8

#define BLOCK_BITS (BLOCK_SIZE * 8) // size of block bitmap (bits)
#define BLOCK_INODES                                                           \
    (BLOCK_SIZE / sizeof(inode_desc_t)) // inode amount per block
#define BLOCK_DENTRIES                                                         \
    (BLOCK_SIZE / sizeof(dentry_t))              // dentry amount per block
#define BLOCK_INDEXES (BLOCK_SIZE / sizeof(u16)) // 块索引数量

#define DIRECT_BLOCK (7)              // 直接块数量
#define INDIRECT1_BLOCK BLOCK_INDEXES // 一级间接块数量
#define INDIRECT2_BLOCK (INDIRECT1_BLOCK * INDIRECT1_BLOCK) // 二级间接块数量
#define TOTAL_BLOCK                                                            \
    (DIRECT_BLOCK + INDIRECT1_BLOCK + INDIRECT2_BLOCK) // 全部块数量

#define SEPARATOR1 '/'
#define SEPARATOR2 '\\'
#define IS_SEPARATOR(c) (c == SEPARATOR1 || c == SEPARATOR2)

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

typedef struct inode_desc_t {
    u16 mode;    // file type and attribute (rwx bit)
    u16 uid;     // user id
    u32 size;    // file size (bytes)
    u32 mtime;   // modified time stamp
    u8 gid;      // group id
    u8 nlinks;   // link amount (how many files point to this inode)
    u16 zone[9]; // direct (0-6), indirect (7) or double indirect (8)
} inode_desc_t;

typedef struct inode_t {
    inode_desc_t *desc;
    struct buffer_t *buf;
    dev_t dev;
    idx_t nr;         // inode number
    u32 count;        // reference count
    time_t atime;     // access time
    time_t ctime;     // change time
    list_node_t node; // node store in super_block_t's inode_list
    dev_t mount;
} inode_t;

// super block
typedef struct super_desc_t {
    u16 inodes;        // inode amount
    u16 zones;         // block amount
    u16 imap_blocks;   // block amount occupied by inode bitmap
    u16 zmap_blocks;   // block amount occupied by logic block bitmap
    u16 firstdatazone; // first data block number
    u16 log_zone_size; // log2(data block amount per logic block)
    u32 max_size;      // file max size
    u16 magic;         // magic
} super_desc_t;

typedef struct super_block_t {
    super_desc_t *desc;
    struct buffer_t *buf;
    struct buffer_t *imaps[IMAP_NR];
    struct buffer_t *zmaps[ZMAP_NR];
    dev_t dev;
    u32 count;
    list_t inode_list; // list contains the inode read to memory yet
    inode_t *iroot;    // inode of root directory
    inode_t *imount;
} super_block_t;

// directory
typedef struct dentry_t {
    u16 nr;              // inode
    char name[NAME_LEN]; // file name
} dentry_t;

typedef struct file_t {
    inode_t *inode; // 文件 inode
    u32 count;      // 引用计数
    off_t offset;   // 文件偏移
    int flags;      // 文件标记
    int mode;       // 文件模式
} file_t;

typedef dentry_t dirent_t;

typedef enum whence_t {
    SEEK_SET = 1, // 直接设置偏移
    SEEK_CUR,     // 当前位置偏移
    SEEK_END      // 结束位置偏移
} whence_t;

super_block_t *get_super(dev_t dev);
super_block_t *read_super(dev_t dev);

idx_t balloc(dev_t dev);          // allocate a file block
void bfree(dev_t dev, idx_t idx); // release a file block
idx_t ialloc(dev_t dev);          // allocate an inode
void ifree(dev_t dev, idx_t idx); // release inode

// 获取 inode 第 block 块的索引值
// 如果不存在 且 create 为 true，则创建
idx_t bmap(inode_t *inode, idx_t block, bool create);

inode_t *get_root_inode();               // 获取根目录 inode
inode_t *iget(dev_t dev, idx_t nr);      // 获得设备 dev 的 nr inode
void iput(inode_t *inode);               // 释放 inode
inode_t *new_inode(dev_t dev, idx_t nr); // 创建新 inode

inode_t *named(char *pathname, char **next); // 获取 pathname 对应的父目录 inode
inode_t *namei(char *pathname);              // 获取 pathname 对应的 inode

// 打开文件，返回 inode
inode_t *inode_open(char *pathname, int flag, int mode);

// 从 inode 的 offset 处，读 len 个字节到 buf
int inode_read(inode_t *inode, char *buf, u32 len, off_t offset);

// 从 inode 的 offset 处，将 buf 的 len 个字节写入磁盘
int inode_write(inode_t *inode, char *buf, u32 len, off_t offset);

// release all file blocks in inode
void inode_truncate(inode_t *inode);

file_t *get_file();
void put_file(file_t *file);

#endif // !OAK_FS_H
