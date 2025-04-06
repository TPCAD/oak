#include "oak/debug/kdebug.h"
#include <oak/cpu.h>
#include <oak/kprintf.h>
#include <oak/syscall.h>
#include <oak/types.h>

void idle_thread() {
    cpu_set_intr_state(true);
    u32 count = 0;
    while (true) {
        // KDEBUG("idle task %d\n", count++);
        asm volatile("sti\n"
                     "hlt\n");
        yield();
    }
}

void init_thread() {
    cpu_set_intr_state(true);
    u32 count = 0;
    while (true) {
        // KDEBUG("init task %d\n", count++);
        sleep(1000);
    }
}

u32 test_thread() {
    cpu_set_intr_state(true);

    u32 count = 0;
    while (true) {
        // kprintf("test task %d\n", count++);
        sleep(2000);
    }
}
