#include "oak/cpu.h"
#include "oak/mm/memory.h"
#include "oak/mm/vmm.h"
#include "oak/task.h"
#include <oak/debug/kassert.h>
#include <oak/debug/kdebug.h>
#include <oak/mm/paging.h>
#include <oak/mm/pmm.h>
#include <oak/string.h>

static u32 KERNEL_PAGE_TABLE[] = {0x2000, 0x3000, 0x4000, 0x5000};

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
 *  @brief  拷贝一页内存到新的物理页
 *  @param  page  要拷贝的页的虚拟地址
 *  @return  新的物理页地址
 *
 *  在开启分页时没有对地址为 0 的页进行映射，因此页表地址 `0xffc00000` 是一个
 *  没有映射的页表项。这里将该页表项暂时映射到新的物理页，方便在分页机制下进行
 *  拷贝。
 */
static u32 paging_copy_page(void *page) {
    u32 paddr = (u32)pmm_alloc_page();
    page_entry_t *page_tbl_addr = (page_entry_t *)PT_VADDR(0);
    entry_init(page_tbl_addr, IDX(paddr), PG_ATTR_PWU);
    flush_tlb(0);
    memcpy((void *)0, (void *)page, PAGE_SIZE);

    *page_tbl_addr &= ~PG_PRESENT;
    flush_tlb(0);
    return paddr;
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
    cpu_set_cr3((u32)page_dir_addr);
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

    // 递归映射页目录本身
    page_dir_addr[1023] = PDE(page_dir_addr, PG_ATTR_PWU | PG_CACHE_DISABLE);

    for (size_t page_dir_idx = (sizeof((KERNEL_PAGE_TABLE)) / 4);
         page_dir_idx < 1023; page_dir_idx++) {
        page_entry_t *page_dir_entry = &page_dir_addr[page_dir_idx];

        if (!PG_IS_PRESENT(*page_dir_entry)) {
            continue;
        }

        page_entry_t *page_tbl_addr = (page_entry_t *)PT_VADDR(page_dir_idx);

        for (size_t page_tbl_idx = 0; page_tbl_idx < 1024; page_tbl_idx++) {
            page_entry_t *page_tbl_entry = &page_tbl_addr[page_tbl_idx];

            if (!PG_IS_PRESENT(*page_tbl_entry)) {
                continue;
            }

            kassert(pmm_page_ref_status(IDX(*page_tbl_entry)));

            *page_tbl_entry &= ~0x2;
            pmm_inc_page_ref(IDX(*page_tbl_entry));

            kassert(pmm_page_ref_status(IDX(*page_tbl_entry)) < 255);
        }

        u32 paddr = paging_copy_page(page_tbl_addr);
        *page_dir_entry = paddr | (*page_dir_entry & 0x00000fff);
    }

    cpu_set_cr3(curr_task->pde);

    return page_dir_addr;
}

/**
 *  @brief  释放当前页目录
 */
void paging_free_pde() {
    task_t *curr_task = task_current_running();
    kassert(curr_task->uid != KERNEL_USER);

    page_entry_t *page_dir_addr = (page_entry_t *)PD_BASE_VADDR;

    for (size_t page_dir_idx = (sizeof(KERNEL_PAGE_TABLE) / 4);
         page_dir_idx < 1023; page_dir_idx++) {
        page_entry_t *page_dir_entry = &page_dir_addr[page_dir_idx];
        if (!PG_IS_PRESENT(*page_dir_entry)) {
            continue;
        }

        page_entry_t *page_tbl_addr = (page_entry_t *)PT_VADDR(page_dir_idx);

        for (size_t page_tbl_idx = 0; page_tbl_idx < 1024; page_tbl_idx++) {
            page_entry_t *page_tbl_entry = &page_tbl_addr[page_tbl_idx];
            if (!PG_IS_PRESENT(*page_tbl_entry)) {
                continue;
            }
            kassert(pmm_page_ref_status(IDX(*page_tbl_addr)));
            // pmm_free_page((void *)*page_tbl_entry);
            vmm_unmap_page((void *)VADDR(page_dir_idx, page_tbl_idx, 0));
        }

        // pmm_free_page((void *)*page_dir_entry);
        vmm_unmap_page((void *)page_dir_entry);
    }
}

#define PF_PRESENT(err) ((err) & 0x1)
#define PF_WRITE(err) ((err) & 0x2)
#define PF_USER(err) ((err) & 0x4)

void page_fault_handler(u32 vector, u32 edi, u32 esi, u32 ebp, u32 esp, u32 ebx,
                        u32 edx, u32 ecx, u32 eax, u32 gs, u32 fs, u32 es,
                        u32 ds, u32 vector0, u32 err_code, u32 eip, u32 cs,
                        u32 eflags) {
    kassert(vector == 0xe);
    u32 missed_vaddr = cpu_get_cr2();
    // KDEBUG("Fault address 0x%p\n", missed_vaddr);
    kassert(missed_vaddr >= KERNEL_MEM_END && missed_vaddr < USER_STACK_BOTTOM);
    task_t *curr_task = task_current_running();

    // 因写只读页造成缺页异常。
    // 创建子进程时，父进程和子进程的页表项都被置为只读。当父进程或子进程写对应
    // 物理页时会触发缺页异常，若该页引用计数大于 1 则为当前进程拷贝该物理页，
    // 若引用计数为 1 则直接将该页置为可写。
    if (PF_PRESENT(err_code)) {
        kassert(PF_WRITE(err_code));

        page_entry_t *page_tbl_addr =
            (page_entry_t *)PT_VADDR(DIDX(missed_vaddr));
        page_entry_t *page_tbl_entry = &page_tbl_addr[TIDX(missed_vaddr)];

        kassert(PG_IS_PRESENT(*page_tbl_entry));
        kassert(pmm_page_ref_status(IDX(*page_tbl_entry)));

        if (pmm_page_ref_status(IDX(*page_tbl_entry)) == 1) {
            *page_tbl_entry |= PG_WRITE;
        } else {
            void *page = (void *)PAGE_ALIGN(missed_vaddr);
            u32 paddr = paging_copy_page(page);
            pmm_dec_page_ref(IDX(*page_tbl_entry));
            entry_init(page_tbl_entry, IDX(paddr), PG_ATTR_PWU);
            flush_tlb(missed_vaddr);
        }

        return;
    }

    // 堆栈因为页面不存在造成缺页异常
    if (!PF_PRESENT(err_code) &&
            (missed_vaddr < (u32)curr_task->user_heap.brk) ||
        (missed_vaddr >= USER_STACK_TOP)) {
        vmm_map_page((void *)missed_vaddr);
        return;
    }

    kpanic("[mm] Page fault. Fault address 0x%p\n", missed_vaddr);
}
