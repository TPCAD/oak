#ifndef OAK_BUFFER_H
#define OAK_BUFFER_H

#include "oak/list.h"
#include "oak/mutex.h"
#include <oak/types.h>

#define BLOCK_SIZE 1024                       // buffer 块大小
#define SECTOR_SIZE 512                       // 扇区大小
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE) // buffer 块内扇区数

typedef struct buffer_t {
    char *data;            // 数据区指针
    u32 dev;               // 设备号
    u32 block;             // 块号，磁盘扇区号摸 2
    int count;             // 引用计数
    list_node_t hash_node; // 哈希表结点
    list_node_t free_node; // 空闲链表结点
    lock_t lock;           // 锁
    bool dirty;            // 是否有修改
    bool valid;            // 是否有效
} buffer_t;

buffer_t *buffer_read(u32 dev, u32 block);
void buffer_write(buffer_t *buf);
void buffer_release(buffer_t *buf);

#endif // !OAK_BUFFER_H
