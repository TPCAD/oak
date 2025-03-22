#ifndef OAK_MM_VMM_H
#define OAK_MM_VMM_H

#include <oak/mm/memory.h>

page_entry_t *vmm_init_pd();

void *vmm_map_phy_page(void *vaddr, void *paddr, page_attr_t dir_attr,
                       page_attr_t tbl_attr);
void vmm_unmap_page(void *vaddr);

void *vmm_alloc_page(void *vaddr, page_attr_t dir_attr, page_attr_t tbl_attr);
// void vmm_free_page();

#endif // !OAK_MM_VMM_H
