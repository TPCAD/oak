#ifndef OAK_SYSCALL_H
#define OAK_SYSCALL_H

#include <oak/types.h>

typedef enum syscall_t {
    SYS_NR_TEST,
    SYS_NR_EXIT = 1,
    SYS_NR_FORK = 2,
    SYS_NR_READ = 3,
    SYS_NR_WRITE = 4,
    SYS_NR_OPEN = 5,
    SYS_NR_CLOSE = 6,
    SYS_NR_WAITPID = 7,
    SYS_NR_CREAT = 8,
    SYS_NR_LINK = 9,
    SYS_NR_UNLINK = 10,
    SYS_NR_EXECVE = 11,
    SYS_NR_CHDIR = 12,
    SYS_NR_TIME = 13,
    SYS_NR_MKNOD = 14,
    SYS_NR_STAT = 18,
    SYS_NR_LSEEK = 19,
    SYS_NR_GETPID = 20,
    SYS_NR_MOUNT = 21,
    SYS_NR_UMOUNT = 22,
    SYS_NR_FSTAT = 28,
    SYS_NR_MKDIR = 39,
    SYS_NR_RMDIR = 40,
    SYS_NR_DUP = 41,
    SYS_NR_PIPE = 42,
    SYS_NR_BRK = 45,
    SYS_NR_UMASK = 60,
    SYS_NR_CHROOT = 62,
    SYS_NR_DUP2 = 63,
    SYS_NR_GETPPID = 64,
    SYS_NR_READDIR = 89,
    SYS_NR_MMAP = 90,
    SYS_NR_MUNMAP = 91,
    SYS_NR_SLEEP = 158,
    SYS_NR_YIELD = 162,
    SYS_NR_GETCWD = 183,

    SYS_NR_CLEAR = 200,
    SYS_NR_MKFS = 201,
} syscall_t;

u32 test();
void yield();
void sleep(u32 ms);
i32 write(fd_t fd, char *buf, u32 len);
i32 brk(void *addr);

pid_t getpid();
pid_t getppid();

pid_t fork();

void exit(int status);

#endif // !OAK_SYSCALL_H
