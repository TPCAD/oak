#include "oak/assert.h"
#include "oak/console.h"
#include "oak/debug.h"
#include "oak/interrupt.h"
#include "oak/memory.h"
#include "oak/syscall.h"
#include "oak/task.h"
#include "oak/types.h"
#include <oak/memory.h>

#define SYSCALL_SIZE 256

handler_t syscall_table[SYSCALL_SIZE];

task_t *task = NULL;

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        panic("syscall nr error!");
    }
}

static void sys_default() { panic("syscall not implemented!"); }

static u32 sys_test() { return 255; }

int32 sys_write(fd_t fd, char *buf, u32 len) {
    if (fd == stdout || fd == stderr) {
        return console_write(buf, len);
    }

    // todo
    panic("Not support!");
    return 0;
}

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = sys_default;
    }
    syscall_table[SYS_NR_TEST] = sys_test;
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YIELD] = task_yield;
    syscall_table[SYS_NR_BRK] = sys_brk;

    syscall_table[SYS_NR_GETPID] = sys_getpid;
    syscall_table[SYS_NR_GETPPID] = sys_getppid;

    syscall_table[SYS_NR_WRITE] = sys_write;
}
