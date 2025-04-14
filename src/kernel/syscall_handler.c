#include "oak/buffer.h"
#include "oak/task.h"
#include <oak/debug/kassert.h>
#include <oak/ide.h>
#include <oak/interrupt/idt.h>
#include <oak/kprintf.h>
#include <oak/mm/pmm.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/types.h>
#include <oak/vdevice.h>

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

extern void dir_test();
static u32 test_syscall() {
    // KDEBUG("syscall test...\n");
    dir_test();

    return 255;
}

i32 syscall_write(fd_t fd, char *buf, u32 len) {
    if (fd == stdout || fd == stderr) {
        return vdevice_write((vdevice_search(VDEV_CONSOLE, 0))->dev, buf, len,
                             0, 0);
    }

    kpanic("[intr] Not implemented file descriptor\n");
    return 0;
}

extern void task_yield();
extern void task_sleep(u32 ms);
extern i32 dmm_brk(void *addr);
extern pid_t task_getpid();
extern pid_t task_getppid();
extern pid_t task_fork();
extern void task_exit(int status);
extern pid_t task_waitpid(pid_t pid, i32 *status);
extern time_t syscall_time();
extern mode_t syscall_umask(mode_t mask);

extern int dentry_remove(char *pathname);
extern int dentry_create(char *pathname, int mode);

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = default_syscall;
    }

    syscall_table[SYS_NR_TEST] = test_syscall;
    syscall_table[SYS_NR_YIELD] = task_yield;
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_WRITE] = syscall_write;
    syscall_table[SYS_NR_BRK] = dmm_brk;
    syscall_table[SYS_NR_GETPID] = task_getpid;
    syscall_table[SYS_NR_GETPPID] = task_getppid;
    syscall_table[SYS_NR_FORK] = task_fork;
    syscall_table[SYS_NR_EXIT] = task_exit;
    syscall_table[SYS_NR_WAITPID] = task_waitpid;
    syscall_table[SYS_NR_TIME] = syscall_time;
    syscall_table[SYS_NR_UMASK] = syscall_umask;
    syscall_table[SYS_NR_MKDIR] = dentry_create;
    syscall_table[SYS_NR_RMDIR] = dentry_remove;
}
