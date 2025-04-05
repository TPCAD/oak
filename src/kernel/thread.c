#include <oak/cpu.h>
#include <oak/kprintf.h>
#include <oak/syscall.h>
#include <oak/types.h>

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
