#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/mutex.h>
#include <oak/syscall.h>

lock_t lock;

void idle_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        // DEBUGK("idle task...%d\n", counter++);
        asm volatile("sti\n"
                     "hlt\n");
        yield();
    }
}

void init_thread() {
    lock_init(&lock);
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        lock_acquire(&lock);
        DEBUGK("init task...%d\n", counter++);
        lock_release(&lock);
    }
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        lock_acquire(&lock);
        DEBUGK("test task...%d\n", counter++);
        lock_release(&lock);
    }
}
