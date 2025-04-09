#include "oak/interrupt/idt.h"
#include "oak/mm/vmm.h"
#include "oak/task.h"
#include "oak/tty.h"
#include "oak/types.h"
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

    char *ptr = NULL;
    vmm_map_page((void *)0x1600000);
    ptr = (char *)0x1600000;
    ptr[3] = 0xaa;
    vmm_unmap_page(ptr);
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

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = default_syscall;
    }

    syscall_table[SYS_NR_TEST] = test_syscall;
    syscall_table[SYS_NR_YIELD] = task_yield;
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_WRITE] = syscall_write;
}
