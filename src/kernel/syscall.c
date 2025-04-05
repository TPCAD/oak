#include "oak/debug/kdebug.h"
#include "oak/interrupt/idt.h"
#include <oak/debug/kassert.h>
#include <oak/syscall.h>

handler_t syscall_table[SYSCALL_SIZE];

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        kpanic("[intr] Illegal syscall number...\n");
    }
}

static void default_syscall() {
    kpanic("[intr] Syscall isn't implemented...\n");
}

static u32 test_syscall() {
    KDEBUG("syscall test...\n");
    return 255;
}

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = default_syscall;
    }

    syscall_table[SYS_NR_TEST] = test_syscall;
}
