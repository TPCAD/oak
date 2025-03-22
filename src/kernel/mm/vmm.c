#include <oak/mm/memory.h>
#include <oak/mm/pmm.h>
#include <oak/mm/vmm.h>
#include <oak/string.h>

/**
 *  @brief  分配一页物理页作为页目录并递归映射该物理页
 *
 *  因为分配页时可能会频繁修改页目录，所以将 PCD 置 1，不缓存页目录。
 */
page_entry_t *vmm_init_pd() {
    page_entry_t *page_dir_addr = pmm_alloc_page();
    memset(page_dir_addr, 0, PAGE_SIZE);

    page_dir_addr[1023] = PDE(page_dir_addr, PG_ATTR_PW | PG_CACHE_DISABLE);

    return page_dir_addr;
}

/**
 *  @brief  将指定物理页映射到指定虚拟页
 *  @param  vaddr  虚拟页地址
 *  @param  paddr  物理页地址
 *  @param  dir_attr  页目录属性
 *  @param  tbl_attr  页表属性
 *  @return  虚拟地址，若失败则为 NULL
 *
 *  通过给定的虚拟地址得到其对应的页目录项和页表项。
 *
 *  若页目录项为空则表示该虚拟地址还未被映射，分配一页页表，创建对应页目录项，
 *  页表项。
 *  若页目录项不为空则查看页表项。页表项若为空则创建对应页表项。若页表项不为空则
 *  查看下一个页表项。若当前页目录项不存在空页表项则查看下一个页目录项，直至找到
 *  可用页表项。
 *
 *  若无可用页目录项则映射失败。若分配新页表失败则映射失败。
 */
void *vmm_map_phy_page(void *vaddr, void *paddr, page_attr_t dir_attr,
                       page_attr_t tbl_attr) {
    // 不映射地址 0
    if (!vaddr || !paddr) {
        return NULL;
    }

    u32 page_dir_idx = DIDX(vaddr);
    u32 page_tbl_idx = TIDX(vaddr);
    page_entry_t *page_dir_vaddr = (page_entry_t *)PD_BASE_VADDR;

    page_entry_t *page_dir_entry = &page_dir_vaddr[page_dir_idx];
    page_entry_t *page_tbl_vaddr = (page_entry_t *)PT_VADDR(page_dir_idx);

    while (page_dir_entry && page_dir_idx < 1024) {
        // 当前页目录项已满，查找下一个页目录项
        if (page_tbl_idx == 1024) {
            page_dir_idx++;
            page_tbl_idx = 0;
            page_dir_entry = &page_dir_vaddr[page_dir_idx];
            page_tbl_vaddr = (page_entry_t *)PT_VADDR(page_dir_idx);
        }

        // 页表有空位，创建页表项
        if (page_tbl_idx && !page_tbl_vaddr[page_tbl_idx]) {
            page_tbl_vaddr[page_tbl_idx] = PTE(paddr, tbl_attr);
            return (void *)VADDR(page_dir_idx, page_tbl_idx, PIDX(vaddr));
        }

        // 当前页表项已有映射，继续查找后续页表项
        page_tbl_idx++;
    }

    // 所有页目录和页表都已满，无可用虚拟内存
    if (page_dir_idx > 1024) {
        return NULL;
    }

    // 页目录未满，分配一页新的页表
    page_entry_t *new_page_tbl_phy_addr = pmm_alloc_page();

    // 无可用物理内存
    if (!new_page_tbl_phy_addr) {
        return NULL;
    }

    // 创建新的页目录项并初始化页表
    page_dir_vaddr[page_dir_idx] = PDE(new_page_tbl_phy_addr, dir_attr);
    memset((void *)PT_VADDR(new_page_tbl_phy_addr), 0, PAGE_SIZE);

    // 创建新的页表项
    page_tbl_vaddr[page_tbl_idx] = PTE(paddr, tbl_attr);

    // 返回最终映射的虚拟地址
    return (void *)VADDR(page_dir_idx, page_tbl_idx, PIDX(vaddr));
}

/**
 *  @brief  删除指定虚拟地址的映射
 *  @param  vaddr  虚拟页地址
 */
void vmm_unmap_page(void *vaddr) {
    u32 page_dir_idx = DIDX(vaddr);
    u32 page_tbl_idx = TIDX(vaddr);
    page_entry_t *page_dir_vaddr = (page_entry_t *)PD_BASE_VADDR;

    page_entry_t page_dir_entry = page_dir_vaddr[page_dir_idx];

    if (page_dir_entry) {
        page_entry_t *page_tbl_vaddr = (page_entry_t *)PT_VADDR(page_dir_idx);
        page_entry_t page_tbl_entry = page_tbl_vaddr[page_tbl_idx];

        if (PG_IS_PRESENT(page_tbl_entry) &&
            !pmm_free_page((void *)page_tbl_entry)) {
            asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
        }
        page_tbl_vaddr[page_tbl_idx] = 0;
    }
}

/**
 *  @brief  为指定虚拟地址分配物理页并建立映射
 *  @param  vaddr  虚拟页地址
 *  @param  dir_attr  页目录属性
 *  @param  tbl_attr  页表属性
 *  @return  虚拟地址，若失败则为 NULL
 *
 *  若指定的虚拟地址已被使用则寻找新的虚拟地址
 */
void *vmm_alloc_page(void *vaddr, page_attr_t dir_attr, page_attr_t tbl_attr) {
    void *paddr = pmm_alloc_page();
    void *result = vmm_map_phy_page(vaddr, paddr, dir_attr, tbl_attr);
    if (!result) {
        pmm_free_page(paddr);
    }
    return result;
}
