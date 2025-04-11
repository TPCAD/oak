#include "oak/ide.h"
#include "oak/interrupt/idt.h"
#include "oak/kprintf.h"
#include "oak/mm/pmm.h"
#include "oak/tty.h"
#include "oak/types.h"
#include <oak/debug/kassert.h>
#include <oak/string.h>
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

extern ide_ctrl_t controllers[2];
static u32 test_syscall() {
    // KDEBUG("syscall test...\n");

    u16 *buf = (u16 *)pmm_alloc_kpage();
    kprintf("pio read buffer 0x%p\n", buf);
    ide_disk_t *disk = &controllers[0].disks[0];
    ide_pio_read(disk, buf, 4, 0);

    memset(buf, 0x5a, 512);

    ide_pio_write(disk, buf, 1, 1);
    kprintf("pio write buffer 0x%p\n", buf);

    pmm_free_kpage(buf);
    return 255;
}

i32 syscall_write(fd_t fd, char *buf, u32 len) {
    if (fd == stdout || fd == stderr) {
        return tty_write_str(buf, len);
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
}
