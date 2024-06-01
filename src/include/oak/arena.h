#ifndef OAK_ARENA_H
#define OAK_ARENA_H

#include <oak/list.h>
#include <oak/types.h>

#define DESC_COUNT 7

typedef list_node_t block_t;

typedef struct arena_descriptor_t {
    u32 total_block;  // block amount in a page
    u32 block_size;   // block size
    list_t free_list; // free bolck list
} arena_descriptor_t;

typedef struct arena_t {
    arena_descriptor_t *desc; // descriptor
    u32 count;                // left blocks or pages
    u32 large;                // larger than 1024 bytes
    u32 magic;
} arena_t;

void *kmalloc(size_t size);
void kfree(void *ptr);
#endif // !OAK_ARENA_H
