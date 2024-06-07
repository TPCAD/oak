#include <oak/assert.h>
#include <oak/buffer.h>
#include <oak/debug.h>
#include <oak/device.h>
#include <oak/list.h>
#include <oak/memory.h>
#include <oak/mutex.h>
#include <oak/string.h>
#include <oak/task.h>
#include <oak/types.h>

#define HASH_COUNT 31

static buffer_t *buffer_start = (buffer_t *)KERNEL_BUFFER_MEM;
static u32 buffer_count = 0;

static buffer_t *buffer_ptr = (buffer_t *)KERNEL_BUFFER_MEM;

static void *buffer_data =
    (void *)(KERNEL_BUFFER_MEM + KERNEL_BUFFER_SIZE - BLOCK_SIZE);

static list_t free_list;              // 缓存链表，被释放的块
static list_t wait_list;              // 等待进程链表
static list_t hash_table[HASH_COUNT]; // 缓存哈希表

u32 hash(dev_t dev, idx_t block) { return (dev ^ block) % HASH_COUNT; }

static buffer_t *get_from_hash_table(dev_t dev, idx_t block) {
    u32 idx = hash(dev, block);
    list_t *list = &hash_table[idx];
    buffer_t *bf = NULL;

    for (list_node_t *node = list->head.next; node != &list->tail;
         node = node->next) {
        buffer_t *ptr = element_entry(buffer_t, hnode, node);
        if (ptr->dev == dev && ptr->block == block) {
            bf = ptr;
            break;
        }
    }

    if (!bf) {
        return NULL;
    }

    if (list_search(&free_list, &bf->rnode)) {
        list_remove(&bf->rnode);
    }

    return bf;
}

static void hash_locate(buffer_t *bf) {
    u32 idx = hash(bf->dev, bf->block);
    list_t *list = &hash_table[idx];
    assert(!list_search(list, &bf->hnode));
    list_push(list, &bf->hnode);
}

static void hash_remove(buffer_t *bf) {
    u32 idx = hash(bf->dev, bf->block);
    list_t *list = &hash_table[idx];
    assert(list_search(list, &bf->hnode));
    list_remove(&bf->hnode);
}

static buffer_t *get_new_buffer() {
    buffer_t *bf = NULL;
    if ((u32)buffer_ptr + sizeof(buffer_t) < (u32)buffer_data) {
        bf = buffer_ptr;
        bf->data = buffer_data;
        bf->dev = EOF;
        bf->block = 0;
        bf->count = 0;
        bf->dirty = false;
        bf->valid = false;
        lock_init(&bf->lock);
        buffer_count++;
        buffer_ptr++;
        buffer_data -= BLOCK_SIZE;
        DEBUGK("buffer count %d\n", buffer_count);
    }

    return bf;
}

static buffer_t *get_free_buffer() {
    buffer_t *bf = NULL;
    while (true) {
        bf = get_new_buffer();
        if (bf) {
            return bf;
        }

        if (!list_empty(&free_list)) {
            bf = element_entry(buffer_t, rnode, list_popback(&free_list));
            hash_remove(bf);
            bf->valid = false;
            return bf;
        }
        task_block(running_task(), &wait_list, TASK_BLOCKED);
    }
}

buffer_t *getblk(dev_t dev, idx_t block) {
    buffer_t *bf = get_from_hash_table(dev, block);
    if (bf) {
        assert(bf->valid);
        return bf;
    }

    bf = get_free_buffer();
    assert(bf->count == 0);
    assert(bf->dirty == 0);

    bf->count = 1;
    bf->dev = dev;
    bf->block = block;
    hash_locate(bf);
    return bf;
}

buffer_t *bread(dev_t dev, idx_t block) {
    buffer_t *bf = getblk(dev, block);
    assert(bf != NULL);
    if (bf->valid) {
        bf->count++;
        return bf;
    }

    device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0,
                   REQ_READ);

    bf->dirty = false;
    bf->valid = true;
    return bf;
}

void bwrite(buffer_t *bf) {
    assert(bf);
    if (!bf->dirty) {
        return;
    }

    device_request(bf->dev, bf->data, BLOCK_SECS, bf->block * BLOCK_SECS, 0,
                   REQ_WRITE);
    bf->dirty = false;
    bf->valid = true;
}

void brelse(buffer_t *bf) {
    if (!bf) {
        return;
    }

    if (bf->dirty) {
        bwrite(bf);
    }

    bf->count--;
    assert(bf->count >= 0);
    if (bf->count) {
        return;
    }

    assert(!bf->rnode.next);
    assert(!bf->rnode.prev);
    list_push(&free_list, &bf->rnode);

    if (!list_empty(&wait_list)) {
        task_t *task = element_entry(task_t, node, list_popback(&wait_list));
        task_unblock(task);
    }
}

void buffer_init() {
    DEBUGK("buffer_t size is %d\n", sizeof(buffer_t));

    list_init(&free_list);
    list_init(&wait_list);

    for (size_t i = 0; i < HASH_COUNT; i++) {
        list_init(&hash_table[i]);
    }
}
