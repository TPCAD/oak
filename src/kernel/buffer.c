/* 内核内存 8M 到 12M 是高速缓冲区，主要用作 CPU 与硬盘之间的缓冲，读写硬盘都会
 * 先读写对应的缓冲区。
 *
 * 缓冲区低地址是 buffer_t 数组，高地址则是对应的数据区，数据区向低地址增长。
 * 读写硬盘到缓冲区以块（Block）为单位，一个块 1024 字节，也就是两个扇区。
 *
 * @a buffer_start 指针指示 buffer_t 数组的起始位置，也就是 8M。
 * @a buffer_ptr 指针指示 buffer_t 数组最后一个元素的地址，动态变化。
 * @a buffer_count 记录 buffer_t 的数量。
 *
 * @a buffer_data 指针指示最新可用数据区的位置，初始置于
 * KERNEL_BUFFER_ADDR + KERNEL_BUFFER_SIZE - BLOCK_SIZE。
 * */
#include <oak/buffer.h>
#include <oak/debug/kassert.h>
#include <oak/list.h>
#include <oak/mm/memory.h>
#include <oak/mutex.h>
#include <oak/task.h>
#include <oak/vdevice.h>

#define HASH_COUNT 31

// 记录 buffer_t 结构体的起始位置，数量及最新位置
static buffer_t *buffer_start = (buffer_t *)KERNEL_BUFFER_ADDR;
static u32 buffer_count = 0;
static buffer_t *buffer_ptr = (buffer_t *)KERNEL_BUFFER_ADDR;

// 记录数据缓冲区位置
static void *buffer_data =
    (void *)(KERNEL_BUFFER_ADDR + KERNEL_BUFFER_SIZE - BLOCK_SIZE);

static list_t free_list; // 空闲链表，已被初始化但空闲的 buffer_t
static list_t wait_list; // 请求可用缓冲区的进程链表
/* 哈希表中存放的 buffer 都是有效的，也就是说 buffer 的内容与硬盘相同，可以直接
 * 使用。获取 buffer 时也是优先从哈希表中寻找。*/
static list_t hash_table[HASH_COUNT];

/**
 *  @brief  计算哈希值
 *  @param  dev  设备号
 *  @param  block  块号
 *  @return  哈希值
 */
static u32 hash(u32 dev, u32 block) { return (dev ^ block) & HASH_COUNT; }

/**
 *  @brief  在哈希表中搜索 buffer
 *  @param  dev  设备号
 *  @param  block  块号
 *  @return  buffer_t 指针
 */
static buffer_t *hash_search(u32 dev, u32 block) {
    u32 idx = hash(dev, block);
    list_t *list = &hash_table[idx];
    buffer_t *buf = NULL;

    for (list_node_t *node = list->head.next; node != &list->tail;
         node = node->next) {
        buffer_t *ptr = element_entry(buffer_t, hash_node, node);

        if (ptr->dev == dev && ptr->block == block) {
            buf = ptr;
            break;
        }
    }

    if (!buf) {
        return NULL;
    }

    // 若 buffer 在空闲链表中，将其删除
    if (list_is_node_exist(&free_list, &buf->free_node)) {
        list_remove(&buf->free_node);
    }

    return buf;
}

/**
 *  @brief  将 buffer 放入哈希表
 *  @param  buf  buffer 指针
 */
static void hash_locate(buffer_t *buf) {
    u32 idx = hash(buf->dev, buf->block);
    list_t *list = &hash_table[idx];
    kassert(!list_is_node_exist(list, &buf->hash_node));
    list_push(list, &buf->hash_node);
}

/**
 *  @brief  将 buffer 从哈希表中删除
 *  @param  buf  buffer 指针
 */
static void hash_remove(buffer_t *buf) {
    u32 idx = hash(buf->dev, buf->block);
    list_t *list = &hash_table[idx];
    kassert(list_is_node_exist(list, &buf->hash_node));
    list_remove(&buf->hash_node);
}

/**
 *  @brief  创建一个 buffer
 *  @return  新的 buffer 指针
 *
 *  如果内存足够（(u32)buffer_ptr + sizeof(buffer_t) <
 * (u32)buffer_data）则直接创建一个 buffer_t
 */
static buffer_t *create_buffer() {
    buffer_t *buf = NULL;

    if ((u32)buffer_ptr + sizeof(buffer_t) < (u32)buffer_data) {
        buf = buffer_ptr;
        buf->data = buffer_data;
        buf->dev = EOF;
        buf->count = 0;
        buf->block = 0;
        buf->dirty = false;
        buf->valid = false;
        lock_init(&buf->lock);
        buffer_count++;
        buffer_ptr++;
        buffer_data -= BLOCK_SIZE;
    }
    return buf;
}

/**
 *  @brief  获取空闲 buffer
 *  @return  buffer_t 指针
 */
static buffer_t *search_free_buffer() {
    buffer_t *buf = NULL;
    while (true) {
        // 内存足够则创建新的 buffer
        buf = create_buffer();
        if (buf) {
            return buf;
        }

        // 内存不足，查看是否有空闲的 buffer
        if (!list_is_empty(&free_list)) {
            buf = element_entry(buffer_t, free_node, list_popback(&free_list));
            hash_remove(buf);
            buf->valid = false;
            return buf;
        }

        // 无空闲 buffer，等待 buffer 被释放
        task_block(task_current_running(), &wait_list, TASK_BLOCKED);
    }
}

/**
 *  @brief  获取 buffer
 *  @param  dev  设备号
 *  @param  block  块号
 *  @return  buffer_t 指针
 */
buffer_t *search_block(u32 dev, u32 block) {
    buffer_t *buf = hash_search(dev, block);
    if (buf) {
        kassert(buf->valid);
        return buf;
    }

    buf = search_free_buffer();
    kassert(buf->count == 0);
    kassert(buf->dirty == 0);

    buf->count = 1;
    buf->dev = dev;
    buf->block = block;
    hash_locate(buf);
    return buf;
}

/**
 *  @brief  将磁盘读取到缓冲区
 *  @param  dev  设备号
 *  @param  block  块号
 *  @return  buffer_t 指针
 */
buffer_t *buffer_read(u32 dev, u32 block) {
    buffer_t *buf = search_block(dev, block);
    kassert(buf != NULL);
    if (buf->valid) {
        buf->count++;
        return buf;
    }

    vdevice_request(buf->dev, buf->data, BLOCK_SECS, buf->block * BLOCK_SECS, 0,
                    REQ_READ);

    buf->dirty = false;
    buf->valid = true;
    return buf;
}

/**
 *  @brief  将缓冲区写入到磁盘
 *  @param  buffer_t 指针
 */
void buffer_write(buffer_t *buf) {
    kassert(buf);
    lock_acquire(&buf->lock);
    if (!buf->dirty) {
        lock_release(&buf->lock);
        return;
    }

    vdevice_request(buf->dev, buf->data, BLOCK_SECS, buf->block * BLOCK_SECS, 0,
                    REQ_WRITE);

    buf->dirty = false;
    buf->valid = true;
    lock_release(&buf->lock);
}

/**
 *  @brief  释放缓冲区
 *  @param  buffer_t 指针
 */
void buffer_release(buffer_t *buf) {
    if (!buf) {
        return;
    }

    buf->count--;
    kassert(buf->count >= 0);

    if (buf->count) {
        return;
    }

    if (buf->free_node.next) {
        list_remove(&buf->free_node);
    }
    // 引用为 0，加入空闲链表
    list_push(&free_list, &buf->free_node);

    if (buf->dirty) {
        buffer_write(buf);
    }

    // 唤醒需要使用缓冲区的任务
    if (!list_is_empty(&wait_list)) {
        task_unblock(element_entry(task_t, node, list_popback(&wait_list)));
    }
}

void buffer_init() {
    list_init(&free_list);
    list_init(&wait_list);

    for (size_t i = 0; i < HASH_COUNT; i++) {
        list_init(&hash_table[i]);
    }
}
