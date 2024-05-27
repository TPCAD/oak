#include "oak/assert.h"
#include "oak/interrupt.h"
#include "oak/syscall.h"
#include "oak/task.h"
#include "oak/types.h"

#define SYSCALL_SIZE 64

extern void task_yield();

handler_t syscall_table[SYSCALL_SIZE];

task_t *task = NULL;

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        panic("syscall nr error!");
    }
}

static void sys_default() { panic("syscall not implemented!"); }

static u32 sys_test() {
    if (!task) {
        task = running_task();
        task_block(task, NULL, TASK_BLOCKED);
    } else {
        task_unblock(task);
        task = NULL;
    }
    return 255;
}

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = sys_default;
    }
    syscall_table[SYS_NR_TEST] = sys_test;
    syscall_table[SYS_NR_YIELD] = task_yield;
}
