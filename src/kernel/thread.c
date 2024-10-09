#include <oak/arena.h>
#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/printk.h>
#include <oak/stdio.h>
#include <oak/stdlib.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/task.h>
#include <oak/types.h>

extern u32 keyboard_read(char *buf, u32 count);
extern void dev_init();

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
    char tmp[100];
    dev_init();
    task_to_user_mode();
}

void test_thread() {
    set_interrupt_state(true);

    // test();
    // mkdir("/world.txt", 0755);
    // rmdir("empty");
    // link("/hello.txt", "/world.txt");
    // unlink("/hello.txt");
    while (true) {
        // printk("A");
        // test();
        sleep(10);
    }
}

void foo_thread() {
    set_interrupt_state(true);
    while (true) {
        printk("B");
        // test();
        // sleep(10);
    }
}
