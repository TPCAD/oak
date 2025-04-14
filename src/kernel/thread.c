#include "oak/debug/kdebug.h"
#include "oak/fs/minix.h"
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

void user_init_thread() {
    char buf[256];
    chroot("/d1");
    chdir("/d2");
    getcwd(buf, sizeof(buf));
    printf("current work directory: %s\n", buf);

    while (true) {
        char ch;
        read(stdin, &ch, 1);
        write(stdout, &ch, 1);
        sleep(10);
    }
}

extern void switch_to_user_mode(target_t target);
void init_thread() {
    // cpu_set_intr_state(true);

    /* `switch_to_user_mode()` 有许多局部变量在栈内，为了防止其在执行过程修改这
     * 些局部变量，需要留出足够的空间。
     * */
    char temp[100];
    switch_to_user_mode(user_init_thread);
}

u32 test_thread() {
    cpu_set_intr_state(true);
    // kprintf("test started of task %d\n", getpid());
    // test();
    // kprintf("test finished of task %d\n", getpid());

    u32 count = 0;
    while (true) {
        BMB;
        // kprintf("test task %d\n", count++);
        sleep(1000);
    }
}
