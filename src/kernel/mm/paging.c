#include "oak/mm/vmm.h"
#include "oak/task.h"
#include <oak/debug/kassert.h>
#include <oak/debug/kdebug.h>
#include <oak/mm/paging.h>
#include <oak/mm/pmm.h>
#include <oak/string.h>

static u32 KERNEL_PAGE_TABLE[] = {0x2000, 0x3000, 0x4000, 0x5000};

static u32 get_cr2() { asm volatile("movl %cr2, %eax\n"); }

static u32 get_cr3() { asm volatile("movl %cr3, %eax\n"); }

static void set_cr3(u32 page_dir_addr) {
    ASSERT_PAGE(page_dir_addr);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(page_dir_addr));
}

static void enable_paging() {
    asm volatile("movl %cr0, %eax\n"
                 "orl $0x80000000, %eax\n"
                 "movl %eax, %cr0\n");
}

/**
 *  @brief  初始化页目录项或页表项
 *  @param  entry  页目录项或页表项
 *  @param  index  页表索引或页索引
 *  @param  pg_attr  页目录或页表的属性
 */
static void entry_init(page_entry_t *entry, u32 index, u32 attr) {
    *entry = 0;
    *entry = (*entry) | index << 12 | attr;
}

/**
 *  @brief  初始化内核页表并开启分页
 */
void paging_init() {
    page_entry_t *page_dir_addr = (page_entry_t *)KERNEL_PAGE_DIR_ADDR;
    memset(page_dir_addr, 0, PAGE_SIZE);

    u32 index = 0;
    for (u32 page_dir_idx = 0;
         page_dir_idx < (sizeof(KERNEL_PAGE_TABLE) / sizeof(u32));
         page_dir_idx++) {
        // 页表所在页清 0
        page_entry_t *page_tbl_addr =
            (page_entry_t *)KERNEL_PAGE_TABLE[page_dir_idx];
        memset(page_tbl_addr, 0, PAGE_SIZE);

        // 初始化页目录项
        page_entry_t *page_dir_entry = &page_dir_addr[page_dir_idx];
        // TODO: kernel memory protection
        entry_init(page_dir_entry, IDX((u32)page_tbl_addr),
                   PG_ATTR_PWU); // 初始化页表项
        for (u32 page_tbl_idx = 0; page_tbl_idx < 1024;
             page_tbl_idx++, index++) {
            if (index == 0) {
                continue;
            }

            page_entry_t *page_tbl_entry = &page_tbl_addr[page_tbl_idx];
            // TODO: kernel memory protection
            entry_init(page_tbl_entry, index, PG_ATTR_PWU);

            // 将内核占用的页标记为已占用
            // pmm_mark_page_occupied(index);
        }
    }

    // 最后一个页目录项指向页目录本身
    page_entry_t *last_page_dir_entry = &page_dir_addr[1023];
    entry_init(last_page_dir_entry, IDX(KERNEL_PAGE_DIR_ADDR), PG_ATTR_PWU);

    // 开启分页
    set_cr3((u32)page_dir_addr);
    enable_paging();
}

/**
 *  @brief  拷贝当前任务的页目录
 *  @return  新的页目录的虚拟地址（内核地址）
 */
page_entry_t *paging_copy_pde() {
    task_t *curr_task = task_current_running();
    page_entry_t *page_dir_addr = pmm_alloc_kpage();
    memcpy(page_dir_addr, (void *)curr_task->pde, PAGE_SIZE);

    page_dir_addr[1023] = PDE(page_dir_addr, PG_ATTR_PWU | PG_CACHE_DISABLE);

    return page_dir_addr;
}

#define PF_PRESENT(err) ((err) & 0x1)
#define PF_WRITE(err) ((err) & 0x2)
#define PF_USER(err) ((err) & 0x4)

void page_fault_handler(u32 vector, u32 edi, u32 esi, u32 ebp, u32 esp, u32 ebx,
                        u32 edx, u32 ecx, u32 eax, u32 gs, u32 fs, u32 es,
                        u32 ds, u32 vector0, u32 err_code, u32 eip, u32 cs,
                        u32 eflags) {
    kassert(vector == 0xe);
    u32 missed_vaddr = get_cr2();
    // KDEBUG("Fault address 0x%p\n", missed_vaddr);
    kassert(missed_vaddr >= KERNEL_MEM_END && missed_vaddr < USER_STACK_BOTTOM);
    task_t *curr_task = task_current_running();

    if (!PF_PRESENT(err_code) && (missed_vaddr >= USER_STACK_TOP)) {
        vmm_map_page((void *)missed_vaddr);
        return;
    }

    kpanic("[mm] Page fault. Fault address 0x%p\n", missed_vaddr);
}
