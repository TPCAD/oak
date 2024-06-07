#ifndef OAK_BUFFER_H
#define OAK_BUFFER_H

#include <oak/list.h>
#include <oak/mutex.h>
#include <oak/types.h>
#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE)

typedef struct buffer_t {
    char *data;        // data
    dev_t dev;         // device number
    idx_t block;       // block number
    int count;         // reference times
    list_node_t hnode; // hash
    list_node_t rnode; // buffer node
    lock_t lock;
    bool dirty;
    bool valid;
} buffer_t;

buffer_t *getblk(dev_t dev, idx_t block);
buffer_t *bread(dev_t dev, idx_t block);
void bwrite(buffer_t *bf);
void brelse(buffer_t *bf);
#endif // !OAK_BUFFER_H
