#include "oak/debug/kdebug.h"
#include <oak/cpu.h>
#include <oak/kprintf.h>
#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/task.h>
#include <oak/types.h>

void idle_thread() {
    cpu_set_intr_state(true);
    u32 count = 0;
    while (true) {
        // KDEBUG("idle task %d\n", count++);
        asm volatile("sti\n"
                     "hlt\n");
        yield();
    }
}

extern int ash_main();
void user_init_thread() {
    clear();
    while (true) {
        i32 status = 0;
        pid_t pid = fork();
        if (pid) {
            pid_t child = waitpid(pid, &status);
            printf("wait pid %d status %d %d\n", child, status, time());
        } else {
            ash_main();
        }
        sleep(1000);
    }
}

extern void switch_to_user_mode(target_t target);
extern void devfile_init();
void init_thread() {
    // cpu_set_intr_state(true);

    /* `switch_to_user_mode()` 有许多局部变量在栈内，为了防止其在执行过程修改这
     * 些局部变量，需要留出足够的空间。
     * */
    char temp[100];
    devfile_init();
    switch_to_user_mode(user_init_thread);
}

u32 test_thread() {
    cpu_set_intr_state(true);

    u32 count = 0;
    while (true) {
        // kprintf("%d", task_current_running()->pid);
        // u32 count = 100000000;
        // while (count--) {
        // }
        sleep(1000);
    }
}
