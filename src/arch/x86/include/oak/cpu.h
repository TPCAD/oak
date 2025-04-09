#ifndef OAK_CPU_H
#define OAK_CPU_H

#include <oak/types.h>

bool cpu_diable_intr();
bool cpu_get_intr_state();
void cpu_set_intr_state(bool state);

u32 cpu_get_cr3();

u32 cpu_get_cr2();

void cpu_set_cr3(u32 page_dir_addr);

void flush_tlb(u32 vaddr);

#endif // !OAK_CPU_H
