#ifndef OAK_CPU_H
#define OAK_CPU_H

#include <oak/types.h>

bool cpu_diable_intr();
bool cpu_get_intr_state();
void cpu_set_intr_state(bool state);

void flush_tlb(u32 vaddr);

#endif // !OAK_CPU_H
