#include "oak/printk.h"
#include "oak/stdlib.h"
#include "oak/task.h"
#include "oak/types.h"
#include <oak/arena.h>
#include <oak/debug.h>
#include <oak/interrupt.h>
#include <oak/stdio.h>
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
    int status;
    while (true) {

        // pid_t pid = fork();
        // if (pid) {
        //     printf("fork after parent %d %d %d\n", pid, get_pid(),
        //     get_ppid()); pid_t child = (waitpid(pid, &status)); printf("wait
        //     pid %d status %d %d\n", child, status, time());
        // } else {
        //     printf("fork after child %d %d %d\n", pid, get_pid(),
        //     get_ppid());
        //     // sleep(1000);
        //     exit(0);
        // }
        // hang();
        sleep(1000);
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
    // set_interrupt_state(true);
    // test();
    task_to_user_mode(user_init_thread);
}

void test_thread() {
    set_interrupt_state(true);
    u32 counter = 0;

    while (true) {
        // printf("test thread %d %d %d\n", get_pid(), get_ppid(), counter++);

        sleep(2000);
        // DEBUGK("test task...%d\n", counter++);
        // BMB;
        // sleep(709);
    }
}
