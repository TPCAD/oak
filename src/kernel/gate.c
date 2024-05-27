#include "oak/assert.h"
#include "oak/debug.h"
#include "oak/interrupt.h"
#include "oak/types.h"
#define SYSCALL_SIZE 64

handler_t syscall_table[SYSCALL_SIZE];

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        panic("syscall nr error!");
    }
}

static void sys_default() { panic("syscall not implemented!"); }

static void sys_test() { DEBUGK("syscall test\n"); }

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = sys_default;
    }
    syscall_table[0] = sys_test;
}
