#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/syscall.h>

void idle_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        DEBUGK("idle task...%d\n", counter++);
        asm volatile("sti\n"
                     "hlt\n");
        yield();
    }
}

void init_thread() {
    set_interrupt_state(true);

    while (true) {
        DEBUGK("init task...\n");
        // test();
    }
}
