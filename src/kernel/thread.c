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
    char buf[256];
    fd_t fd;
    int len = 0;
    fd = open("/hello.txt", O_RDWR, 0755);
    len = read(fd, buf, sizeof(buf));
    printf("hello.txt content: %s length %d\n", buf, len);
    close(fd);

    fd = open("/world.txt", O_CREAT | O_RDWR, 0755);
    len = write(fd, buf, len);
    while (true) {
        char ch;
        read(stdin, &ch, 1);
        write(stdout, &ch, 1);
        sleep(10);
    }
}

void init_thread() {
    char tmp[100];
    task_to_user_mode(user_init_thread);
}

void test_thread() {
    set_interrupt_state(true);

    test();
    // mkdir("/world.txt", 0755);
    // rmdir("empty");
    // link("/hello.txt", "/world.txt");
    // unlink("/hello.txt");
    while (true) {
        test();
        sleep(10);
    }
}
