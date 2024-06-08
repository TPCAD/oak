#include <oak/arena.h>
#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/printk.h>
#include <oak/stdio.h>
#include <oak/stdlib.h>
#include <oak/syscall.h>
#include <oak/task.h>
#include <oak/types.h>

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
    int status;
    while (true) {
        sleep(1000);
    }
}

void init_thread() {
    char tmp[100];
    task_to_user_mode(user_init_thread);
}

void test_thread() {
    set_interrupt_state(true);

    // mkdir("/world.txt", 0755);
    // rmdir("empty");
    link("/hello.txt", "/world.txt");
    unlink("/hello.txt");
    while (true) {
        test();
        sleep(10);
    }
}
