#include "oak/printk.h"
#include "oak/task.h"
#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/syscall.h>

extern u32 keyboard_read(char *buf, u32 count);

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

void static user_init_thread() {
    u32 counter = 0;
    char ch;

    while (true) {
        BMB;
        sleep(100);
        // DEBUGK("hello\n");
    }
}

void init_thread() {
    // set_interrupt_state(true);
    // u32 counter = 0;
    //
    // char ch = 0;
    // while (true) {
    //     // DEBUGK("init task...%d\n", counter++);
    //     bool intr = interrupt_diable();
    //     keyboard_read(&ch, 1);
    //     printk("%c", ch);
    //     set_interrupt_state(intr);
    //     // sleep(500);
    // }
    char tmp[100];
    task_to_user_mode(user_init_thread);
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        DEBUGK("test task...%d\n", counter++);
        sleep(709);
    }
}
