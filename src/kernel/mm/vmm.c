#include <oak/cpu.h>
#include <oak/debug/kassert.h>
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
 *  @brief  检查虚拟地址是否已存在于页表中
 *  @param  vaddr  虚拟地址
 *  @return  存在则返回 0，页目录项不存在则返回 1，页表项不存在则返回 2
 */
u32 vmm_is_vaddr_exist(void *vaddr) {
    vaddr = (void *)PAGE_ALIGN(vaddr);

    u32 page_dir_idx = DIDX(vaddr);
    page_entry_t *page_dir_vaddr = (page_entry_t *)PD_BASE_VADDR;

    // 虚拟地址的页目录项不存在
    if (!*(page_dir_vaddr + page_dir_idx)) {
        return 1;
    }

    page_entry_t *page_tbl_vaddr = (page_entry_t *)PT_VADDR(page_dir_idx);
    u32 page_tbl_idx = TIDX(vaddr);

    // 虚拟地址的页表项不存在
    if (!*(page_tbl_vaddr + page_tbl_idx)) {
        return 2;
    }

    // 虚拟地址的页表项存在物理地址，该虚拟地址已映射物理页
    if ((*(page_tbl_vaddr + page_tbl_idx) >> 12)) {
        return 3;
    }

    // 虚拟地址存在于页表，但未映射物理页
    return 0;
}

/**
 *  @brief  删除指定虚拟地址的映射
 *  @param  vaddr  虚拟页地址
 *
 *  虽然删除映射只需要将页表项的地址部分清 0，但使用没有映射的虚拟地址会造成缺页
 *  中断，这与直接清除整个页表项，也就是删除虚拟页并无太大差别，因此删除映射操作
 *  会直接将整个页表项清空。
 */
void vmm_unmap_page(void *vaddr) {
    // 要删除页目录项需要用到三级索引，而一般的地址只需要二级索引。而且页目录项
    // 的虚拟地址并不需要经过一系列转换，可以直接使用。
    if (((u32)vaddr & 0xfffff000) == 0xfffff000) {
        u32 *addr = (u32 *)vaddr;
        if (PG_IS_PRESENT(*addr) && !pmm_free_page((void *)*addr)) {
            *addr = 0;
            flush_tlb((u32)vaddr);
        }
        return;
    }

    u32 page_dir_idx = DIDX(vaddr);
    u32 page_tbl_idx = TIDX(vaddr);
    page_entry_t *page_dir_vaddr = (page_entry_t *)PD_BASE_VADDR;

    if (page_dir_vaddr[page_dir_idx]) {
        page_entry_t *page_tbl_vaddr = (page_entry_t *)PT_VADDR(page_dir_idx);
        page_entry_t page_tbl_entry = page_tbl_vaddr[page_tbl_idx];

        if (PG_IS_PRESENT(page_tbl_entry) &&
            !pmm_free_page((void *)page_tbl_entry)) {
            page_tbl_vaddr[page_tbl_idx] = 0;
            flush_tlb((u32)vaddr);
        }
    }
}

/**
 *  @brief  为指定虚拟地址分配物理页并建立映射
 *  @param  vaddr  虚拟页地址
 *  @return  虚拟地址，若失败则为 NULL
 */
void *vmm_map_page(void *vaddr) {
    u32 page_dir_idx = DIDX(vaddr);
    page_entry_t *page_dir_vaddr = (page_entry_t *)PD_BASE_VADDR;
    u32 page_tbl_idx = TIDX(vaddr);
    page_entry_t *page_tbl_vaddr = (page_entry_t *)PT_VADDR(page_dir_idx);

    // 虚拟地址的页目录项不存在
    if (!page_dir_vaddr[page_dir_idx]) {
        page_entry_t *new_page_tbl_phy_addr = pmm_alloc_page();

        // 无可用物理内存
        if (!new_page_tbl_phy_addr) {
            return NULL;
        }
        // 创建新的页目录项并初始化页表
        page_dir_vaddr[page_dir_idx] = PDE(new_page_tbl_phy_addr, PG_ATTR_PWU);
        memset((void *)PT_VADDR(page_dir_idx), 0, PAGE_SIZE);
    }

    void *paddr = pmm_alloc_page();
    if (!paddr) {
        return NULL;
    }
    page_tbl_vaddr[page_tbl_idx] = PTE(paddr, PG_ATTR_PWU);
    return vaddr;
}

/**
 *  @brief  分配多页虚拟内存
 *  @param  count  页数
 *  @return  起始页虚拟地址
 *
 *  只分配用户内存，内核内存已经在分页初始化时对等映射。
 *
 *  分配一页虚拟内存需要先找到当前所使用的页目录，
 */
void *vmm_alloc_page(u32 count, u32 offset) {
    kassert(count > 0);
    if (offset == 0) {
        offset = KERNEL_MEM_END;
    }
    kassert(offset >= USER_EXEC_ADDR);

    u32 found_pages_count = 0;

    u32 page_dir_idx = offset / PAGE_SIZE / 1024;
    u32 page_tbl_idx = 0;
    page_entry_t *page_dir_vaddr = (page_entry_t *)PD_BASE_VADDR;
    page_entry_t *page_tbl_vaddr = (page_entry_t *)PT_VADDR(page_dir_idx);
    page_entry_t *page_dir_entry = page_dir_vaddr + page_dir_idx;

    while (*page_dir_entry && page_dir_idx < 1024 &&
           found_pages_count != count) {
        if (page_tbl_idx == 1024 && found_pages_count != count) {
            found_pages_count = 0;
            page_dir_idx++;
            page_tbl_idx = 0;
            page_dir_entry = page_dir_vaddr + page_dir_idx;
            page_entry_t *page_tbl_vaddr =
                (page_entry_t *)PT_VADDR(page_dir_idx);
        }

        // 当前页表有空位，创建一个页表项
        if (!page_tbl_vaddr[page_tbl_idx]) {
            found_pages_count++;
        }

        page_tbl_idx++;
    }

    // 所有页目录和页表都已满，无可用虚拟内存
    if (page_dir_idx > 1024) {
        return NULL;
    }

    // 已找到连续的虚拟页
    if (found_pages_count == count) {
        while (found_pages_count--) {
            page_tbl_idx--;
            page_tbl_vaddr[page_tbl_idx] = PTE(0, PG_ATTR_WU);
        }
        return (void *)VADDR(page_dir_idx, page_tbl_idx, 0);
    }

    // 页目录未满，分配一页新的页表
    page_entry_t *new_page_tbl_phy_addr = pmm_alloc_page();

    // 无可用物理内存
    if (!new_page_tbl_phy_addr) {
        return NULL;
    }

    // 创建新的页目录项并初始化页表
    page_dir_vaddr[page_dir_idx] = PDE(new_page_tbl_phy_addr, PG_ATTR_PWU);
    memset((void *)PT_VADDR(page_dir_idx), 0, PAGE_SIZE);

    // 创建新的页表项
    for (u32 i = 0; i < count; i++) {
        page_tbl_vaddr[page_tbl_idx + i] = PTE(0, PG_ATTR_WU);
    }

    // 返回分配的虚拟地址
    return (void *)VADDR(page_dir_idx, page_tbl_idx, 0);
}

/**
 *  @brief  释放一页虚拟页
 *  @param  vaddr  虚拟页地址
 */
void vmm_free_page(void *vaddr) { vmm_unmap_page(vaddr); }

void vmm_alloc_test() {
    void *vaddr = vmm_alloc_page(1, 0);
    kassert((u32)vaddr == KERNEL_MEM_END);
    vmm_free_page(vaddr);
    vaddr = vmm_alloc_page(1, 0);
    kassert((u32)vaddr == KERNEL_MEM_END);
    vmm_free_page(vaddr);

    vaddr = vmm_alloc_page(3, 0);
    for (int i = 0; i < 3; i++) {
        vmm_free_page(vaddr + i * PAGE_SIZE);
    }
}

void vmm_map_test() {
    void *vaddr = vmm_alloc_page(1, 0);
    kassert((u32)vaddr == KERNEL_MEM_END);
    // 缺页异常
    // *(int *)vaddr = 1;
    vmm_map_page(vaddr);
    *(int *)vaddr = 1;
    vmm_unmap_page(vaddr);
}

void vmm_test() {
    vmm_alloc_test();
    vmm_map_test();
}
