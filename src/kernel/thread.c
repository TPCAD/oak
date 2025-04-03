#include "oak/cpu.h"
#include <oak/kprintf.h>
#include <oak/types.h>

u32 thread_a() {

    cpu_set_intr_state(true);

    while (true) {
        kprintf("A");
    }
}

u32 thread_b() {

    cpu_set_intr_state(true);
    while (true) {
        kprintf("B");
    }
}
