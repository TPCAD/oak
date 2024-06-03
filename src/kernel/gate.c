#include "oak/assert.h"
#include "oak/console.h"
#include "oak/debug.h"
#include "oak/ide.h"
#include "oak/interrupt.h"
#include "oak/memory.h"
#include "oak/syscall.h"
#include "oak/task.h"
#include "oak/types.h"
#include <oak/memory.h>
#include <oak/string.h>

#define SYSCALL_SIZE 256

extern time_t sys_time();
extern ide_ctrl_t controllers[2];

handler_t syscall_table[SYSCALL_SIZE];

task_t *task = NULL;

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        panic("syscall nr error!");
    }
}

static void sys_default() { panic("syscall not implemented!"); }

static u32 sys_test() {
    u16 *buf = (u16 *)alloc_kpage(1);
    DEBUGK("pio read buffer 0x%p\n", buf);
    ide_disk_t *disk = &controllers[0].disks[0];
    ide_pio_read(disk, buf, 4, 0);

    memset(buf, 0x5a, 512);
    ide_pio_write(disk, buf, 1, 1);
    free_kpage((u32)buf, 1);
    return 255;
}

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

    syscall_table[SYS_NR_EXIT] = task_exit;
    syscall_table[SYS_NR_FORK] = task_fork;
    syscall_table[SYS_NR_WAITPID] = task_waitpid;

    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_YIELD] = task_yield;
    syscall_table[SYS_NR_BRK] = sys_brk;

    syscall_table[SYS_NR_GETPID] = sys_getpid;
    syscall_table[SYS_NR_GETPPID] = sys_getppid;

    syscall_table[SYS_NR_WRITE] = sys_write;

    syscall_table[SYS_NR_TIME] = sys_time;
}
