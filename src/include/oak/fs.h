#ifndef OAK_FS_H
#define OAK_FS_H

#include "oak/buffer.h"
#include "oak/device.h"
#include <oak/list.h>
#include <oak/types.h>

#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512

#define MINIX1_MAGIC 0x137f
#define NAME_LEN 14

#define IMAP_NR 8
#define ZMAP_NR 8

#define BLOCK_BITS (BLOCK_SIZE * 8) // size of block bitmap (bits)

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
    idx_t nr;     // inode number
    u32 count;    // reference count
    time_t atime; // access time
    time_t ctime; // create time
    list_node_t node;
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
    list_t inode_list;
    inode_t *iroot;
    inode_t *imount;
} super_block_t;

// directory
typedef struct dentry_t {
    u16 nr;              // inode
    char name[NAME_LEN]; // file name
} dentry_t;

super_block_t *get_super(dev_t dev);
super_block_t *read_super(dev_t dev);

idx_t balloc(dev_t dev);          // allocate a file block
void bfree(dev_t dev, idx_t idx); // release a file block
idx_t ialloc(dev_t dev);          // allocate an inode
void ifree(dev_t dev, idx_t idx); // release inode

#endif // !OAK_FS_H
