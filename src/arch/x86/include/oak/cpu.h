#ifndef OAK_CPU_H
#define OAK_CPU_H

#include <oak/types.h>

bool cpu_diable_intr();
bool cpu_get_intr_state();
void cpu_set_intr_state(bool state);

u32 __inline cpu_get_cr3() { asm volatile("movl %cr3, %eax\n"); }

u32 __inline cpu_get_cr2() { asm volatile("movl %cr2, %eax\n"); }

void cpu_set_cr3(u32 page_dir_addr);

void flush_tlb(u32 vaddr);

#endif // !OAK_CPU_H
