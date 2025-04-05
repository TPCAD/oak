#include "oak/debug/kdebug.h"
#include <oak/cpu.h>
#include <oak/kprintf.h>
#include <oak/syscall.h>
#include <oak/types.h>

void idle_thread() {
    cpu_set_intr_state(true);
    u32 count = 0;
    while (true) {
        KDEBUG("idle task %d\n", count++);
        asm volatile("sti\n"
                     "hlt\n");
        yield();
    }
}

void init_thread() {
    cpu_set_intr_state(true);
    while (true) {
        KDEBUG("init task...\n");
        test();
    }
}

u32 thread_a() {

    cpu_set_intr_state(true);

    while (true) {
        kprintf("A");
        test();
    }
}

u32 thread_b() {

    cpu_set_intr_state(true);
    while (true) {
        kprintf("B");
        test();
    }
}

u32 thread_c() {

    cpu_set_intr_state(true);
    while (true) {
        kprintf("C");
        test();
    }
}
