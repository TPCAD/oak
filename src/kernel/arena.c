/*
 *  该文件包含 `kmalloc` 和 `kfree` 两个用于内核堆内存管理的函数。这两个函数的
 *  实现依赖于数据结构 `arena_t`。
 *
 *  在一页中，内存会被分成大小相等的若干块。在页开始的位置会有一个 `arena_t`，
 *  它记录了该页内存的块大小，剩余空闲块数量等信息。`arena_t` 中有嵌套数据结构
 *  `arena_descriptor_t`，它存储了该页的块大小，块总数，以及链表结点。
 *
 *  系统维护一个 arena 描述符数组，该数组对应 7 种不同大小的块。
 **/

#include <oak/arena.h>
#include <oak/assert.h>
#include <oak/list.h>
#include <oak/memory.h>
#include <oak/oak.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/types.h>
// extern u32 free_pages;
static arena_descriptor_t descriptors[DESC_COUNT];

/*
 *  @brief  初始化 arena 描述符数组
 **/
void arena_init() {
    u32 block_size = 16;
    for (size_t i = 0; i < DESC_COUNT; i++) {
        arena_descriptor_t *desc = &descriptors[i];
        desc->block_size = block_size;
        desc->total_block = (PAGE_SIZE - sizeof(arena_t)) / block_size;
        list_init(&desc->free_list);
        block_size <<= 1;
    }
}

/*
 *  @brief  获取 arena 中第 @a idx 块的地址
 *  @param  arena  arena
 *  @param  idx  块索引
 **/
static void *get_arena_block(arena_t *arena, u32 idx) {
    assert(arena->desc->total_block > idx);
    void *addr = (void *)(arena + 1);
    u32 gap = idx * arena->desc->block_size;
    return addr + gap;
}

/*
 *  @brief  获取块所属的 arena 的地址
 *  @param  block  空闲块链表的一个结点
 *
 *  实际上是获取当前页的地址
 **/
static arena_t *get_block_arena(block_t *block) {
    return (arena_t *)((u32)block & 0xfffff000);
}

/*
 *  @brief  堆内存分配
 *  @param  size  内存大小，单位为字节
 **/
void *kmalloc(size_t size) {
    arena_descriptor_t *desc = NULL;
    arena_t *arena;
    block_t *block;
    char *addr;

    // 大于 1024 字节
    if (size > 1024) {
        u32 asize = size + sizeof(arena_t);
        u32 count = div_round_up(asize, PAGE_SIZE);

        arena = (arena_t *)alloc_kpage(count);
        memset(arena, 0, count * PAGE_SIZE);

        arena->large = true;
        arena->count = count;
        arena->desc = NULL;
        arena->magic = OAK_MAGIC;

        addr = (char *)((u32)arena + sizeof(arena_t));
        return addr;
    }

    // 小于等于 1024 字节，在描述符表中寻找对应的描述符
    for (size_t i = 0; i < DESC_COUNT; i++) {
        desc = &descriptors[i];
        if (desc->block_size >= size) {
            break;
        }
    }
    assert(desc != NULL);

    // 描述符的空闲块列表为空
    if (list_empty(&desc->free_list)) {
        arena = (arena_t *)alloc_kpage(1);
        memset(arena, 0, PAGE_SIZE);

        // 初始化该页的 arena
        arena->desc = desc;
        arena->large = false;
        arena->count = desc->total_block;
        arena->magic = OAK_MAGIC;

        // 将该页的所有块加入到空闲块列表中
        for (size_t i = 0; i < desc->total_block; i++) {
            block = get_arena_block(arena, i);
            assert(!list_search(&arena->desc->free_list, block));
            list_push(&arena->desc->free_list, block);
            assert(list_search(&arena->desc->free_list, block));
        }
    }

    // BUG: 从列表中取出一个空闲块返回，但不能保证程序不会破坏该链表结点保存的
    // 信息，而一旦被破坏，该块将再也找不到。
    block = list_pop(&desc->free_list);

    arena = get_block_arena(block);
    assert(arena->magic == OAK_MAGIC && !arena->large);

    arena->count--;
    return block;
}

/*
 *  @brief  堆内存释放
 *  @param  ptr  内存地址
 **/
void kfree(void *ptr) {
    assert(ptr);

    block_t *block = (block_t *)ptr;
    arena_t *arena = get_block_arena(block);

    assert(arena->large == 1 || arena->large == 0);
    assert(arena->magic == OAK_MAGIC);

    // 大于 1024 字节，释放连续页
    if (arena->large) {
        free_kpage((u32)arena, arena->count);
        return;
    }

    // BUG: 链表信息可能被破坏
    list_push(&arena->desc->free_list, block);
    arena->count++;

    // 若空闲块数量与描述符的块总数相等，则释放该页内存
    if (arena->count == arena->desc->total_block) {
        for (size_t i = 0; i < arena->desc->total_block; i++) {
            block = get_arena_block(arena, i);
            assert(list_search(&arena->desc->free_list, block));
            list_remove(block);
            assert(!list_search(&arena->desc->free_list, block));
        }
        free_kpage((u32)arena, 1);
    }
}
