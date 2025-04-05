#include "oak/debug/kdebug.h"
#include "oak/interrupt/idt.h"
#include "oak/task.h"
#include <oak/debug/kassert.h>
#include <oak/syscall.h>

#define SYSCALL_SIZE 256
handler_t syscall_table[SYSCALL_SIZE];

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        kpanic("[intr] Illegal syscall number...\n");
    }
}

static void default_syscall() {
    kpanic("[intr] Syscall isn't implemented...\n");
}

task_t *task = NULL;

static u32 test_syscall() {
    // KDEBUG("syscall test...\n");

    if (!task) {
        task = task_current_running();
        task_block(task, NULL, TASK_BLOCKED);
    } else {
        task_unblock(task);
        task = NULL;
    }
    return 255;
}

extern void task_yield();

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = default_syscall;
    }

    syscall_table[SYS_NR_TEST] = test_syscall;
    syscall_table[SYS_NR_YIELD] = task_yield;
}
