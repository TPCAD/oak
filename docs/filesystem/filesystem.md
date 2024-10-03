## Minix 文件系统

Minix 文件系统第一版将磁盘分成若干块，每个块占两个扇区，也就是 1024 字节。第 0 
个块是主引导块，第 1 个块是超级块，接着是 inode 位图块和块位图块。在此之后便是 
inode，inode 之后则是文件真正存储的区域。主引导块起始于第 2049 个扇区（从 1 开
始计数）。

## 文件与目录的存储

- 文件
文件的内容直接存储在硬盘中，不包括文件名等信息。

- 目录
目录的内容以 `dentry` 数据结构的形式存储在硬盘中。将一个块看成一个 `denry` 数组
，数组的每一项就是目录中的每一项。不管是目录还是文件。`dentry` 由 inode 号和文
件/目录名。

## inode 描述符

```c
typedef struct inode_desc_t {
    u16 mode;    // file type and attribute (rwx bit)
    u16 uid;     // user id
    u32 size;    // file size (bytes)
    u32 mtime;   // modified time stamp
    u8 gid;      // group id
    u8 nlinks;   // link amount (how many files point to this inode)
    u16 zone[9]; // direct (0-6), indirect (7) or double indirect (8)
} inode_desc_t;
```

- nlinks
硬链接数，至少为 1。

- zone
zone 字段是一个数组，里面存储的是该 inode 对应的文件所在的具体块号（从第 2048 
个扇区开始，从 0 开始计数）。数组的前 7 个是直接块，即可以直接通过里面的值找到
对应的文件内容。第 8 个是一级间接块，通过里面的内容找到的块所存储的不是文件的具
体内容，而是又一个 zone，该 zone 里面的项才存储了对应的文件所在的块号。第 9 个
是二级间接块，与一级间接块类似，只是又多了一层。
