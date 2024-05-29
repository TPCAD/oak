#ifndef OAK_SYSCALL_H
#define OAK_SYSCALL_H

#include "oak/types.h"

typedef enum syscall_t { SYS_NR_TEST, SYS_NR_SLEEP, SYS_NR_YIELD } syscall_t;

u32 test();
void yield();
void sleep(u32 ms);

#endif // OAK_SYSCALL_H
