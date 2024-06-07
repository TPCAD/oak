#include <oak/assert.h>
#include <oak/buffer.h>
#include <oak/console.h>
#include <oak/debug.h>
#include <oak/device.h>
#include <oak/interrupt.h>
#include <oak/memory.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/task.h>
#include <oak/types.h>

#define SYSCALL_SIZE 256

extern time_t sys_time();
extern mode_t sys_umask();

handler_t syscall_table[SYSCALL_SIZE];

task_t *task = NULL;

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        panic("syscall nr error!");
    }
}

static void sys_default() { panic("syscall not implemented!"); }

static u32 sys_test() {
    extern void dir_test();
    dir_test();

    char ch;
    device_t *device;

    device = device_find(DEV_KEYBOARD, 0);
    assert(device);
    device_read(device->dev, &ch, 1, 0, 0);

    device = device_find(DEV_CONSOLE, 0);
    assert(device);
    device_write(device->dev, &ch, 1, 0, 0);

    return 255;
}

extern int32 console_write(void *dev, char *buf, u32 count);

int32 sys_write(fd_t fd, char *buf, u32 len) {
    if (fd == stdout || fd == stderr) {
        return console_write(NULL, buf, len);
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

    syscall_table[SYS_NR_UMASK] = sys_umask;
}
