#ifndef OAK_DMM_H
#define OAK_DMM_H

#include <oak/types.h>

#define KERNEL_HEAP_INIT_SIZE 4096

#define M_CURR_ALLOCATED 0x1 // 当前块已分配
#define M_PREV_ALLOCATED 0x2 // 上一块已分配
#define M_PREV_FREE 0x0      // 上一块未分配
#define M_CURR_FREE 0x0      // 当前块未分配

#define ALIGN_SIZE 4
#define TAG_SIZE 4

#define KERNEL_HEAP_ADDR 0x700000
#define KERNEL_HEAP_SIZE 0x100000
#define KERNEL_HEAP_END (KERNEL_HEAP_ADDR + KERNEL_HEAP_SIZE)

/*  Tag 占 4 字节，第 0 位表示当前块是否可用，第 1 位表示上一块是否可用，剩下 32
 *  位表示当前块大小，块大小 4 字节对齐。
 * */
#define BUILD_TAG(size, flags) (((size) & ~0x3) | (flags))
#define WRITE_TAG(ptr, tag) (*((u32 *)(ptr)) = (tag))
#define READ_TAG(ptr) (*((u32 *)(ptr)))

// 获取区块的标签内容
#define CHUNK_SIZE(tag) ((tag) & ~0x3)
#define CHUNK_PS(tag) ((tag) & M_PREV_ALLOCATED)
#define CHUNK_CS(tag) ((tag) & M_CURR_ALLOCATED)

#define CHUNK_LAST_TAG(ptr, size) ((u32 *)(ptr + size - TAG_SIZE))

#define NEXT_CHUNK(ptr) ((u8 *)(ptr) + CHUNK_SIZE(READ_TAG(ptr)))

#define REAL_ALLOC_PTR(ptr) ((u8 *)ptr + TAG_SIZE)

typedef struct heap_context_t {
    void *start_addr;
    void *brk;
    void *max_addr;
} heap_context_t;

void kheap_init();

#endif // !OAK_DMM_H
