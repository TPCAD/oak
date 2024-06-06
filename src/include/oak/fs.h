#ifndef OAK_FS_H
#define OAK_FS_H

#include <oak/list.h>
#include <oak/types.h>

#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512

#define MINIX1_MAGIC 0x137f
#define NAME_LEN 14

#define IMAP_NR 8
#define ZMAP_NR 8

typedef struct inode_desc_t {
    u16 mode;    // file type and attribute (rwx bit)
    u16 uid;     // user id
    u32 size;    // file size (bytes)
    u32 mtime;   // modified time stamp
    u8 gid;      // group id
    u8 nlinks;   // link amount (how many files point to this inode)
    u16 zone[9]; // direct (0-6), indirect (7) or double indirect (8)
} inode_desc_t;

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

// directory
typedef struct dentry_t {
    u16 nr;              // inode
    char name[NAME_LEN]; // file name
} dentry_t;

#endif // !OAK_FS_H
