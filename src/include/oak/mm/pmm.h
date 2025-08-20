#ifndef OAK_MM_PMM_H
#define OAK_MM_PMM_H

#include <oak/types.h>

#define PM_MAP_ADDR 0x100000         // 1MB
#define PM_BIT_MAX_SIZE (128 * 1024) // 128KB

void pmm_mark_page_free(u32 ppn);
void pmm_mark_page_occupied(u32 ppn);

void pmm_mark_chunk_free(u32 ppn, u32 count);
void pmm_mark_chunk_occupied(u32 ppn, u32 count);

u32 pmm_page_ref_status(u32 ppn);

void pmm_inc_page_ref(u32 ppn);
void pmm_dec_page_ref(u32 ppn);

void *pmm_alloc_page();
int pmm_free_page(void *page_addr);

void *pmm_alloc_kpage();
int pmm_free_kpage(void *page_addr);

#endif // !OAK_MM_PMM_H
