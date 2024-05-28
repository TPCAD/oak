#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/mutex.h>
#include <oak/syscall.h>

mutex_t mutex;

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
    mutex_init(&mutex);
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        mutex_lock(&mutex);
        DEBUGK("init task...%d\n", counter++);
        mutex_unlock(&mutex);
    }
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        mutex_lock(&mutex);
        DEBUGK("test task...%d\n", counter++);
        mutex_unlock(&mutex);
    }
}
