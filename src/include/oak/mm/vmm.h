#ifndef OAK_MM_VMM_H
#define OAK_MM_VMM_H

#include <oak/mm/memory.h>

page_entry_t *vmm_init_pd();

void *vmm_map_page(void *vaddr);
void vmm_unmap_page(void *vaddr);

void *vmm_alloc_page(u32 count, u32 offset);
void vmm_free_page(void *vaddr);

#endif // !OAK_MM_VMM_H
