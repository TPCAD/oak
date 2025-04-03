/*  pmm.c 定义用于管理物理内存的各种函数。
 *
 *  物理内存的可用状态由 u8 数组进行管理，每个物理页可被引用 255 次。
 *
 *  */

#include <oak/debug/kassert.h>
#include <oak/mm/memory.h>
#include <oak/mm/pmm.h>
#include <oak/string.h>
#include <oak/types.h>

#define START_PAGE IDX(KERNEL_MEM_END)
#define START_KPAGE IDX(MEMORY_BASE)

/* 物理内存位图置于 0x100000（1M） */
u8 *pm_map = (u8 *)PM_MAP_ADDR;

/*  可用物理页的数量，由最大物理内存地址给出。 */
u32 max_pg = 0;

/*  可用内核物理页的数量，由内核结束地址给出。 */
u32 max_kpg = 0;

/*  最近分配的物理页号，用于在分配物理页时减少搜索时间。
 *  内核物理页的起始页数是 1M 内存，但因为地址 0x0 是空指针，所以不使用页号
 *  0，初始化为 1。
 *  用户物理页的起始页数是内核结束地址。
 *  */
u32 recent_alloc_page = 1;

u32 recent_alloc_kpage = 1;

/**
 *  @brief  标记一页物理页为可用
 *  @param  ppn  物理页号（paddr >> 12）
 *
 *  慎用该函数，它会将所有引用清空。
 */
void pmm_mark_page_free(u32 ppn) { pm_map[ppn] = 0; }

/**
 *  @brief  标记一页物理页为已占用
 *  @param  ppn  物理页号（paddr >> 12）
 *
 *  慎用该函数，它会将所有引用清空。
 */
void pmm_mark_page_occupied(u32 ppn) { pm_map[ppn] = 1; }

/**
 *  @brief  标记连续物理页为可用
 *  @param  ppn  起始物理页号
 *  @param  count  页数
 *
 *  慎用该函数，它会将所有引用清空。
 */
void pmm_mark_chunk_free(u32 ppn, u32 count) {
    for (size_t i = 0; i < count; i++) {
        pm_map[ppn + i] = 0;
    }
}

/**
 *  @brief  标记连续物理页为已占用
 *  @param  ppn  起始物理页号
 *  @param  count  页数
 *
 *  慎用该函数，它会将所有引用清空。
 */
void pmm_mark_chunk_occupied(u32 ppn, u32 count) {
    for (size_t i = 0; i < count; i++) {
        pm_map[ppn + i] = 1;
    }
}

/**
 *  @brief  初始化物理内存数组
 *  @param  mem_upper_lim  物理内存最大地址
 *
 *  所有物理页初始化为已占用。最近分配物理页初始化为 1。
 *  可用物理页数由物理内存最大地址给出。
 */
void pmm_init(u32 mem_upper_lim) {
    max_pg = IDX((mem_upper_lim));
    recent_alloc_page = START_PAGE;

    max_kpg = START_PAGE;
    recent_alloc_kpage = START_KPAGE;

    // 物理内存数组占用的内存页数
    u32 occupied_pages =
        (max_pg % PAGE_SIZE) ? (max_pg / PAGE_SIZE) + 1 : (max_pg / PAGE_SIZE);

    memset(pm_map, 1, occupied_pages * PAGE_SIZE);
}

/**
 *  @brief  分配一页物理页
 *  @return  物理页地址
 *
 *  从 [recent_alloc_page, upper_lim) 寻找可用物理页，若没有则从
 *  [START_PAGE, old_alloc_page) 寻找，若仍然没有则返回 NULL。
 */
void *pmm_alloc_page() {
    u32 old_alloc_page = recent_alloc_page;
    u32 upper_lim = max_pg;
    void *found_page = NULL;

    while (!found_page && recent_alloc_page < upper_lim) {
        if (pm_map[recent_alloc_page] == 0) {
            pmm_mark_page_occupied(recent_alloc_page);
            found_page = (void *)(recent_alloc_page << 12);
            break;
        } else {
            recent_alloc_page++;

            if (recent_alloc_page >= upper_lim &&
                old_alloc_page != START_PAGE) {
                upper_lim = old_alloc_page;
                old_alloc_page = START_PAGE;
                recent_alloc_page = START_PAGE;
            }
        }
    }

    kassert(IDX(found_page) >= max_kpg && IDX(found_page) <= max_pg);
    return found_page;
}

/**
 *  @brief  释放一页物理页
 *  @param  page_addr  物理页地址
 *  @return  成功则返回 0，失败则返回 1
 */
int pmm_free_page(void *page_addr) {
    kassert(IDX(page_addr) >= max_kpg && IDX(page_addr) <= max_pg);
    u32 pg = IDX(page_addr);
    if (pg && pg < max_pg) {
        pm_map[pg] == 1 ? pmm_mark_page_free(pg) : pm_map[pg]--;
        return 0;
    }
    return 1;
}

/**
 *  @brief  分配一页内核物理页
 *  @return  物理页地址
 *
 *  从 [recent_alloc_kpage, max_kpg) 寻找可用内核物理页，若没有则从
 *  [MEMORY_BASE, old_alloc_kpage) 寻找，若仍然没有则返回 NULL。
 */
void *pmm_alloc_kpage() {
    u32 old_alloc_kpage = recent_alloc_kpage;
    u32 upper_lim = max_kpg;
    void *found_kpage = NULL;

    while (!found_kpage && recent_alloc_kpage < upper_lim) {
        if (pm_map[recent_alloc_kpage] == 0) {
            pmm_mark_page_occupied(recent_alloc_kpage);
            found_kpage = (void *)(recent_alloc_kpage << 12);
            break;
        } else {
            recent_alloc_kpage++;

            if (recent_alloc_kpage >= upper_lim &&
                old_alloc_kpage != START_KPAGE) {
                upper_lim = old_alloc_kpage;
                old_alloc_kpage = START_KPAGE;
                recent_alloc_kpage = START_KPAGE;
            }
        }
    }

    kassert(IDX(found_kpage) < max_kpg);
    return found_kpage;
}

/**
 *  @brief  释放一页内核物理页
 *  @param  page_addr  物理页地址
 *  @return  成功则返回 0，失败则返回 1
 */
int pmm_free_kpage(void *page_addr) {
    kassert(IDX(page_addr) >= START_KPAGE && IDX(page_addr) < max_kpg);
    u32 pg = IDX(page_addr);
    if (pg && pg < max_pg) {
        pm_map[pg] == 1 ? pmm_mark_page_free(pg) : pm_map[pg]--;
        return 0;
    }
    return 1;
}
