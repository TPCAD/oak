#include "oak/buffer.h"
#include "oak/fs/minix.h"
#include "oak/task.h"
#include <oak/debug/kassert.h>
#include <oak/ide.h>
#include <oak/interrupt/idt.h>
#include <oak/kprintf.h>
#include <oak/mm/pmm.h>
#include <oak/string.h>
#include <oak/syscall.h>
#include <oak/types.h>
#include <oak/vdevice.h>

#define SYSCALL_SIZE 256
handler_t syscall_table[SYSCALL_SIZE];

void syscall_check(u32 nr) {
    if (nr >= SYSCALL_SIZE) {
        kpanic("[intr] Illegal syscall number...\n");
    }
}

static void default_syscall() {
    kpanic("[intr] Syscall isn't implemented...\n");
}

extern void dir_test();
static u32 test_syscall() {
    // KDEBUG("syscall test...\n");
    inode_t *inode = inode_open("/world.txt", O_RDWR | O_CREAT, 0755);
    kassert(inode);

    char *buf = (char *)pmm_alloc_kpage();
    int i = inode_read(inode, buf, 1024, 0);

    memset(buf, 'A', 4096);
    inode_write(inode, buf, 4096, 0);

    inode_free(inode);

    return 255;
}

extern void task_yield();
extern void task_sleep(u32 ms);
extern i32 dmm_brk(void *addr);
extern pid_t task_getpid();
extern pid_t task_getppid();
extern pid_t task_fork();
extern void task_exit(int status);
extern pid_t task_waitpid(pid_t pid, i32 *status);
extern time_t syscall_time();
extern mode_t syscall_umask(mode_t mask);

extern int dentry_remove(char *pathname);
extern int dentry_create(char *pathname, int mode);
extern int file_link(char *oldname, char *newname);
extern int file_unlink(char *pathname);
extern fd_t file_open(char *filename, int flags, int mode);
extern fd_t file_create(char *filename, int mode);
extern void file_close(fd_t fd);
extern int file_read(fd_t fd, char *buf, int len);
extern int file_write(fd_t fd, char *buf, int len);
int file_lseek(fd_t fd, i32 offset, whence_t whence);

int task_chdir(char *pathname);
int task_chroot(char *pathname);
char *task_getcwd(char *buf, size_t size);

void syscall_init() {
    for (size_t i = 0; i < SYSCALL_SIZE; i++) {
        syscall_table[i] = default_syscall;
    }

    syscall_table[SYS_NR_TEST] = test_syscall;
    syscall_table[SYS_NR_YIELD] = task_yield;
    syscall_table[SYS_NR_SLEEP] = task_sleep;
    syscall_table[SYS_NR_WRITE] = file_write;
    syscall_table[SYS_NR_READ] = file_read;
    syscall_table[SYS_NR_BRK] = dmm_brk;
    syscall_table[SYS_NR_GETPID] = task_getpid;
    syscall_table[SYS_NR_GETPPID] = task_getppid;
    syscall_table[SYS_NR_FORK] = task_fork;
    syscall_table[SYS_NR_EXIT] = task_exit;
    syscall_table[SYS_NR_WAITPID] = task_waitpid;
    syscall_table[SYS_NR_TIME] = syscall_time;
    syscall_table[SYS_NR_UMASK] = syscall_umask;
    syscall_table[SYS_NR_MKDIR] = dentry_create;
    syscall_table[SYS_NR_RMDIR] = dentry_remove;
    syscall_table[SYS_NR_LINK] = file_link;
    syscall_table[SYS_NR_UNLINK] = file_unlink;
    syscall_table[SYS_NR_OPEN] = file_open;
    syscall_table[SYS_NR_CREAT] = file_create;
    syscall_table[SYS_NR_CLOSE] = file_close;
    syscall_table[SYS_NR_LSEEK] = file_lseek;
    syscall_table[SYS_NR_CHDIR] = task_chdir;
    syscall_table[SYS_NR_CHROOT] = task_chroot;
    syscall_table[SYS_NR_GETCWD] = task_getcwd;
}
