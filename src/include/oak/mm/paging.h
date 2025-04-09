#ifndef OAK_MM_PAGING_H
#define OAK_MM_PAGING_H

#include <oak/mm/memory.h>

void paging_init();

page_entry_t *paging_copy_pde();

#endif // !OAK_MM_PAGING_H
