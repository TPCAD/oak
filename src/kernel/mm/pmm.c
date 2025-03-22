/*  pmm.c 定义用于管理物理内存的各种函数。
 *
 *  物理内存的可用状态由位图进行管理，从高位进行开始记录，例如位图第一个字节为
 *  0xf0，表示物理内存的前 4 个页已被占用。
 *
 *  32 位系统的最大可访问内存为 4GB，共有 `4G / 4K = 1M` 页，需要的位图大小为
 *  `1M / 8 = 128KB`，共 32 页。
 *
 *  */

#include <oak/mm/memory.h>
#include <oak/mm/pmm.h>
#include <oak/string.h>
#include <oak/types.h>

/* 物理内存位图置于 0x100000（1M） */
u8 *pm_bitmap = (u8 *)PM_BITMAP_ADDR;

/*  可用物理页的数量，由最大物理内存地址给出。 */
u32 max_pg = 0;

/*  最近分配的物理页号，用于在分配物理页时减少搜索时间。因为地址 0x0 是空指针，
 *  所以不使用页号 0，初始化为 1，*/
u32 recent_alloc_page = 1;

/**
 *  @brief  标记一页物理页为未占用
 *  @param  ppn  物理页号（paddr >> 12）
 *
 *  1. 物理页号除以 8 得到字节偏移量，模 8 得到位偏移量
 *  2. 0b10000000 左移位偏移量得到掩码
 *  3. 位图对应字节和掩码的非值逻辑与将对应为置为 0
 */
void pmm_mark_page_free(u32 ppn) {
    u32 byte_offset = ppn / 8;
    u32 bit_offset = ppn % 8;
    u32 mask = 0x80 >> bit_offset;
    pm_bitmap[byte_offset] = pm_bitmap[byte_offset] & ~mask;
}

/**
 *  @brief  标记一页物理页为已占用
 *  @param  ppn  物理页号（paddr >> 12）
 */
void pmm_mark_page_occupied(u32 ppn) {
    u32 byte_offset = ppn / 8;
    u32 bit_offset = ppn % 8;
    u32 mask = 0x80 >> bit_offset;
    pm_bitmap[byte_offset] = pm_bitmap[byte_offset] | mask;
}

/**
 *  @brief  标记连续物理页为未占用
 *  @param  ppn  起始物理页号
 *  @param  count  页数
 *
 *  将需要处理的字节分为三部分：
 *  1. 起始页所在的字节
 *  2. 起始页后的连续字节
 *  3. 不足一字节的多个比特
 *
 *  当 @a count 与 `bit_offset` 之和小于 8 时，只需处理起始页所在的字节。
 *
 *  当 `(count + bit_offset) / 8` 大于 1 时，需要处理起始页后的连续字节，连续的
 *  字节数为 `((count + bit_offset) / 8) - 1`。
 *
 *  当 `(count + bit_offset) % 8` 大于 0 时，需要处理最后不足一字节的多个比特。
 *  需要处理的位数为 `(count + bit_offset) % 8`。
 *
 *  **起始页所在字节的处理：**
 *
 *  因为从高位开始标记，所以标记连续位就是对起始位及其低位进行标记。需要标记的
 *  位数有以下两种情况：
 *  1. 对于只需处理起始页所在字节的情况，@a count 就是需要处理的位数
 *  2. 对于其他情况， 则需要计算出起始位到字节最低位的差值
 *
 *  因此可以得到计算位数的表达式：
 *
 *  ```c
 *  marked_bits_in_1st_byte =
 *                          (count + bit_offset) < 8 ? count : 8 - bit_offset
 *  ```
 *
 *  接下来是计算掩码。将 1 左移 n 位后减 1 即可得到位于低位的连续 n 个 1。例如
 *  `(1 << 3) - 1 = 0b00000111`。再将这连续的 n 个 1 移动到起始位就得到了掩码。
 *
 *  将这连续的 n 个 1 移动到起始位实际上是将其最高位移动到起始位，而最高位的位置
 *  就是 `marked_bits_in_1st_byte` 的值。
 *  `8 - bit_offset` 得到起始位的位置，再减去 `marked_bits_in_1st_byte` 即可得到
 *  需要移动的位数。最后的表达式为 `8 - bit_offset - marked_bits_in_1st_byte`。
 *
 *  最后将值取非即可得到掩码。
 *
 *  **起始页后连续字节处理：**
 *
 *  连续的字节数由表达式 `(count + bit_offset) / 8` 给出。循环将连续字节置 0
 *  即可。
 *
 *  **剩下不足一字节的比特：**
 *
 *  剩余的比特数由表达式 `(count + bit_offset) % 8`
 *  给出。用同样的方法在低位得到连续的 n 个 1。将其左移移动 `8 - remaining_bits`
 *  后取非即可得到掩码。
 *
 */
void pmm_mark_chunk_free(u32 ppn, u32 count) {
    u32 byte_offset = ppn / 8;
    u32 bit_offset = ppn % 8;

    u32 marked_bits_in_1st_byte =
        (count + bit_offset) < 8 ? count : 8 - bit_offset;

    pm_bitmap[byte_offset] &= ~(((1U << marked_bits_in_1st_byte) - 1)
                                << (8 - bit_offset - marked_bits_in_1st_byte));

    byte_offset++;

    u32 continuous_bytes = (count + bit_offset) / 8;

    for (int i = 0; continuous_bytes != 0 && i < continuous_bytes - 1;
         i++, byte_offset++) {
        pm_bitmap[byte_offset] = 0;
    }

    u32 remaining_bits = (count + bit_offset) % 8;
    pm_bitmap[byte_offset] &=
        ~(((1U << remaining_bits) - 1) << (8 - remaining_bits));
}

/**
 *  @brief  标记连续物理页为已占用
 *  @param  ppn  起始物理页号
 *  @param  count  页数
 */
void pmm_mark_chunk_occupied(u32 ppn, u32 count) {
    u32 byte_offset = ppn / 8;
    u32 bit_offset = ppn % 8;

    u32 marked_bits_in_1st_byte =
        (count + bit_offset) < 8 ? count : 8 - bit_offset;

    pm_bitmap[byte_offset] |= (((1U << marked_bits_in_1st_byte) - 1)
                               << (8 - bit_offset - marked_bits_in_1st_byte));

    byte_offset++;

    u32 continuous_bytes = (count + bit_offset) / 8;

    for (int i = 0; continuous_bytes != 0 && i < continuous_bytes - 1;
         i++, byte_offset++) {
        pm_bitmap[byte_offset] = 0xff;
    }

    u32 remaining_bits = (count + bit_offset) % 8;
    pm_bitmap[byte_offset] |=
        (((1U << remaining_bits) - 1) << (8 - remaining_bits));
}

/**
 *  @brief  初始化物理内存位图
 *  @param  mem_upper_lim  物理内存最大地址
 *
 *  所有物理页初始化为已占用。最近分配物理页初始化为 1。
 *  可用物理页数由物理内存最大地址给出。
 */
void pmm_init(u32 mem_upper_lim) {
    max_pg = IDX((mem_upper_lim));
    recent_alloc_page = START_PAGE;

    memset(pm_bitmap, 0xff, PM_BIT_MAX_SIZE);
}

/**
 *  @brief  分配一页物理页
 *  @return  物理页地址
 */
void *pmm_alloc_page() {
    u32 old_alloc_page = recent_alloc_page;
    u32 upper_lim = max_pg;
    void *found_page = NULL;
    u8 chunk = 0;

    while (!found_page && recent_alloc_page < upper_lim) {
        // recent_alloc_page / 8 得出 recent_alloc_page 所在的字节
        chunk = pm_bitmap[recent_alloc_page >> 3];

        if (chunk != 0xffU) {
            for (int i = recent_alloc_page % 8; i < 8;
                 i++, recent_alloc_page++) {
                if (!(chunk & (0x80 >> i))) {
                    pmm_mark_page_occupied(recent_alloc_page);
                    found_page = (void *)(recent_alloc_page << 12);
                    break;
                }
            }
        } else {
            recent_alloc_page += 8;

            if (recent_alloc_page >= upper_lim &&
                old_alloc_page != START_PAGE) {
                upper_lim = old_alloc_page;
                old_alloc_page = START_PAGE;
                recent_alloc_page = START_PAGE;
            }
        }
    }
    return found_page;
}

/**
 *  @brief  释放一页物理页
 *  @param  page_addr  物理页地址
 *  @return  成功则返回 0，失败则返回 1
 */
int pmm_free_page(void *page_addr) {
    // TODO: assert page_addr
    u32 pg = (u32)page_addr >> 12;
    if (pg && pg < max_pg) {
        pmm_mark_page_free(pg);
        return 0;
    }
    return 1;
}
