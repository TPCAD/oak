#ifndef OAK_MM_PMM_H
#define OAK_MM_PMM_H

#include <oak/types.h>

#define PM_BITMAP_ADDR 0x100000      // 1MB
#define PM_BIT_MAX_SIZE (128 * 1024) // 128KB

#define START_PAGE 1

void pmm_mark_page_free(u32 ppn);
void pmm_mark_page_occupied(u32 ppn);

void pmm_mark_chunk_free(u32 ppn, u32 count);
void pmm_mark_chunk_occupied(u32 ppn, u32 count);

void *pmm_alloc_page();
int pmm_free_page(void *page_addr);

#endif // !OAK_MM_PMM_H
