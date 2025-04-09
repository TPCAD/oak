/*  动态内存管理
 *
 *  堆内存使用隐式链表（Implicit List）管理。堆内存会被分为大小不等的区块
 *  （Chunk），区块要求 4 字节对齐。每个区块由标签（Tag）和有效载荷组成。
 *  标签位于区块的起始位置，大小为 4 字节。标签第 0 位表示当前区块是否空闲，
 *  第 1 位表示上一区块 是否空闲。剩下 30 位表示区块大小。对于空闲的区块，在其末
 *  尾还有一个标签，内容与起始位置的标签相同。
 *
 *  整个堆空间有一个首区块和一个尾区块，它们的大小都是 4 字节，也就是说这两个
 *  区块只有标签，没有有效载荷。首区块的值固定为 `0x00000005`，表示区块大小为
 *  4 字节，当前区块已分配，上一区块已分配。尾区块的值初始固定为 `0x00000003`，
 *  表示区块大小为 0，且当前块已分配，上一区块已分配。
 *
 *  区块的大小包括首标签和有效载荷。用户请求一块内存时，系统会自动为其加上首标签
 *  的大小，也就是说实际分配的内存大小是用户请求内存大小加上首标签的内存大小。
 *
 * */
#include "oak/debug/kassert.h"
#include "oak/kprintf.h"
#include "oak/mm/memory.h"
#include "oak/mm/vmm.h"
#include "oak/stdlib.h"
#include "oak/task.h"
#include "oak/types.h"
#include <oak/mm/dmm.h>

heap_context_t kernel_heap;

// 使用 u8* 是因为有许多需要加减指针的地方，如果用 u32* 那么加减的单位就会是 4
// 字节，而 u8* 刚好是 1 字节
void *dmm_coalesce_chunk(u8 *chunk_ptr) {
    u32 tag = READ_TAG(chunk_ptr);
    u32 prev_stat = CHUNK_PS(tag);
    u32 size = CHUNK_SIZE(tag);
    u32 next_tag = READ_TAG(chunk_ptr + size);

    if (CHUNK_CS(next_tag) && !prev_stat) {
        // case 1: 上一块空闲
        u32 prev_tag = READ_TAG(chunk_ptr - TAG_SIZE);
        u32 prev_size = CHUNK_SIZE(prev_tag);
        u32 new_tag = BUILD_TAG(prev_size + size, CHUNK_PS(prev_tag));
        WRITE_TAG(chunk_ptr - prev_size, new_tag);
        WRITE_TAG(CHUNK_LAST_TAG(chunk_ptr, size), new_tag);
        chunk_ptr -= prev_size;
    } else if (!CHUNK_CS(next_tag) && prev_stat) {
        // case 2: 下一块空闲
        u32 next_size = CHUNK_SIZE(next_tag);
        u32 new_tag = BUILD_TAG(size + next_size, prev_stat);
        WRITE_TAG(chunk_ptr, new_tag);
        WRITE_TAG(CHUNK_LAST_TAG(chunk_ptr, size + next_size), new_tag);
    } else if (!CHUNK_CS(next_tag) && !prev_stat) {
        // case 3: 全部空闲
        u32 prev_tag = READ_TAG(chunk_ptr - TAG_SIZE);
        u32 prev_size = CHUNK_SIZE(prev_tag);
        u32 next_size = CHUNK_SIZE(next_tag);
        u32 new_tag =
            BUILD_TAG(size + prev_size + next_size, CHUNK_PS(prev_tag));
        WRITE_TAG(chunk_ptr - prev_size, new_tag);
        WRITE_TAG(CHUNK_LAST_TAG(chunk_ptr, next_size + size), new_tag);
        chunk_ptr -= prev_size;
    }

    // case 4: 全部不空闲
    return chunk_ptr;
}

void *dmm_grow_kheap(size_t size) {
    if (!size) {
        return kernel_heap.brk;
    }
    void *curr_brk = kernel_heap.brk;
    // 加上的 TAG_SIZE 是尾标签的大小
    void *next = curr_brk + ROUNDUP(size + TAG_SIZE, ALIGN_SIZE);

    if (next >= kernel_heap.max_addr || next < curr_brk) {
        return NULL;
    }

    kernel_heap.brk += size;

    size = ROUNDUP(size, ALIGN_SIZE);

    u32 old_maker = READ_TAG(curr_brk);
    WRITE_TAG(curr_brk, BUILD_TAG(size, CHUNK_PS(old_maker)));
    WRITE_TAG(CHUNK_LAST_TAG(curr_brk, size),
              BUILD_TAG(size, CHUNK_PS(old_maker)));
    WRITE_TAG(NEXT_CHUNK(curr_brk),
              BUILD_TAG(0, M_CURR_ALLOCATED | ~M_PREV_ALLOCATED));

    return dmm_coalesce_chunk(curr_brk);
}

void dmm_place_chunk(u8 *ptr, size_t size) {
    u32 tag = READ_TAG(ptr);
    size_t chunk_size = CHUNK_SIZE(tag);
    // 标记区块已占用
    READ_TAG(ptr) = BUILD_TAG(size, CHUNK_PS(tag) | M_CURR_ALLOCATED);
    u8 *next_chunk = (u8 *)(ptr + size);
    u32 diff = chunk_size - size;

    if (!diff) {
        // 无剩余空间
        u32 next_tag = READ_TAG(next_chunk);
        WRITE_TAG(next_chunk, next_tag & ~M_PREV_ALLOCATED);
    } else {
        // 有剩余空间
        u32 next_tag = BUILD_TAG(diff, M_CURR_FREE | M_PREV_ALLOCATED);
        WRITE_TAG(next_chunk, next_tag);
        WRITE_TAG(CHUNK_LAST_TAG(next_chunk, diff), next_tag);
    }

    dmm_coalesce_chunk(next_chunk);
}

// TODO: 只适用于修改页地址，后续会支持修改任意地址
i32 dmm_brk(void *addr) {
    u32 brk = (u32)addr;
    ASSERT_PAGE(brk);
    kassert(brk > KERNEL_MEM_END && brk < USER_STACK_TOP);

    task_t *curr_task = task_current_running();
    kassert(curr_task->uid != KERNEL_USER);

    u32 old_brk = (u32)curr_task->user_heap.brk;

    if (old_brk > brk) {
        for (u32 page = brk; page < old_brk; page += PAGE_SIZE) {
            vmm_unmap_page((void *)page);
        }
    }

    curr_task->user_heap.brk = addr;
    return 0;
}

void kheap_init() {
    kernel_heap.start_addr = (void *)KERNEL_HEAP_ADDR;
    kernel_heap.brk = kernel_heap.start_addr;
    kernel_heap.max_addr = (void *)KERNEL_HEAP_END;

    WRITE_TAG(kernel_heap.start_addr,
              BUILD_TAG(4, M_PREV_ALLOCATED | M_CURR_ALLOCATED));
    WRITE_TAG(kernel_heap.start_addr + TAG_SIZE,
              BUILD_TAG(0, M_PREV_ALLOCATED | M_CURR_ALLOCATED));
    kernel_heap.brk += TAG_SIZE;

    dmm_grow_kheap(KERNEL_HEAP_INIT_SIZE);
}

void *kmalloc(size_t size) {
    if (!size) {
        return NULL;
    }

    u8 *ptr = kernel_heap.start_addr;
    size = ROUNDUP(size + TAG_SIZE, ALIGN_SIZE);

    while (ptr < (u8 *)kernel_heap.brk) {
        u32 tag = READ_TAG(ptr);
        size_t chunk_size = CHUNK_SIZE(tag);
        // 堆空间已满
        if (!chunk_size && CHUNK_CS(tag)) {
            break;
        }
        // 找到符合的区块
        if (chunk_size >= size && !CHUNK_CS(tag)) {
            dmm_place_chunk(ptr, size);
            return REAL_ALLOC_PTR(ptr);
        }
        ptr += chunk_size;
    }

    // 分配更多的堆空间
    if ((ptr = dmm_grow_kheap(size))) {
        dmm_place_chunk(ptr, size);
        return REAL_ALLOC_PTR(ptr);
    }

    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) {
        return;
    }

    u8 *chunk_ptr = (u8 *)ptr - TAG_SIZE;
    u32 tag = READ_TAG(chunk_ptr);
    size_t size = CHUNK_SIZE(tag);
    u8 *next_tag = chunk_ptr + size;

    kassert((u32)ptr < (u32)(-size) && !((u32)ptr & 0x3));
    kassert(size > TAG_SIZE);

    WRITE_TAG(chunk_ptr, tag & ~M_CURR_ALLOCATED);
    WRITE_TAG(CHUNK_LAST_TAG(chunk_ptr, size), tag & ~M_CURR_ALLOCATED);
    WRITE_TAG(next_tag, READ_TAG(chunk_ptr + size) & ~M_PREV_ALLOCATED);

    dmm_coalesce_chunk(chunk_ptr);
}

void kheap_test() {
    u32 *ptr = kmalloc(sizeof(u32));
    kprintf("%p\n", ptr);
    *ptr = 0x12345678;

    u8 **arr = (u8 **)kmalloc(10 * sizeof(u8 *));
    for (size_t i = 0; i < 10; i++) {
        arr[i] = (u8 *)kmalloc((i + 1) * 2);
    }

    for (size_t i = 0; i < 10; i++) {
        kfree(arr[i]);
    }

    u8 *big = kmalloc(8192);
    big[0] = 123;
    big[1] = 12;
    big[2] = 1;

    kprintf("%p: %d\n", &big[0], big[0]);
    kprintf("%p: %d\n", &big[1], big[1]);
    kprintf("%p: %d\n", &big[2], big[2]);

    kfree(ptr);
    kfree(big);
}
