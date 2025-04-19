#include "oak/buffer.h"
#include "oak/debug/kdebug.h"
#include "oak/fs/minix.h"
#include "oak/mm/dmm.h"
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

static u32 test_syscall() {
    // KDEBUG("syscall test...\n");
    char *chunk0 = kmalloc(sizeof(char));
    kprintf("chunk0: %p\n", chunk0);
    char *chunk1 = kmalloc(sizeof(char));
    kprintf("chunk1: %p\n", chunk1);
    char *chunk2 = kmalloc(sizeof(char));
    kprintf("chunk2: %p\n", chunk2);

    *chunk0 = 0xaa;
    *chunk1 = 0x55;
    *chunk2 = 0x5a;
    kassert(*chunk0 == (char)0xaa);
    kassert(*chunk1 == (char)0x55);
    kassert(*chunk2 == (char)0x5a);
    kprintf("memory content assert success\n");

    kfree(chunk0);
    kfree(chunk1);
    kfree(chunk2);
    kprintf("free memory success\n");

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
extern int file_lseek(fd_t fd, i32 offset, whence_t whence);

extern int task_chdir(char *pathname);
extern int task_chroot(char *pathname);
extern char *task_getcwd(char *buf, size_t size);

extern int file_readdir(fd_t fd, dentry_t *dir, u32 count);

extern void tty_clear();

extern int file_stat(char *filename, stat_t *statbuf);
extern int file_fstat(fd_t fd, stat_t *statbuf);

extern int inode_build_devfile_node(char *filename, int mode, int dev);

extern int super_mount(char *devname, char *dirname, int flags);
extern int super_umount(char *target);

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
    syscall_table[SYS_NR_READDIR] = file_readdir;
    syscall_table[SYS_NR_CLEAR] = tty_clear;
    syscall_table[SYS_NR_STAT] = file_stat;
    syscall_table[SYS_NR_FSTAT] = file_fstat;
    syscall_table[SYS_NR_MKNOD] = inode_build_devfile_node;
    syscall_table[SYS_NR_MOUNT] = super_mount;
    syscall_table[SYS_NR_UMOUNT] = super_umount;
}
