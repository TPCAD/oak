#ifndef OAK_SYSCALL_H
#define OAK_SYSCALL_H

#include <oak/stat.h>
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

enum mmap_type_t {
    PROT_NONE = 0,
    PROT_READ = 1,
    PROT_WRITE = 2,
    PROT_EXEC = 4,

    MAP_SHARED = 1,
    MAP_PRIVATE = 2,
    MAP_FIXED = 0x10,
};

u32 test();
pid_t fork();
void exit(int status);
pid_t waitpid(pid_t pid, int32 *status);
void yield();
int execve(char *filename, char *argv[], char *envp[]);
void sleep(u32 ms);
pid_t get_pid();
pid_t get_ppid();
int brk(void *addr);
void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset);
int munmap(void *addr, size_t length);
fd_t open(char *filename, int flags, int mode);
fd_t creat(char *filename, int mode);
void close(fd_t fd);
fd_t dup(fd_t oldfd);
fd_t dup2(fd_t oldfd, fd_t newfd);
int read(fd_t fd, char *buf, int len);
int write(fd_t fd, char *buf, int len);
int lseek(fd_t fd, off_t offset, int whence);
int readdir(fd_t fd, void *dir, int count);
char *getcwd(char *buf, size_t size);
int chdir(char *pathname);
int chroot(char *pathname);
int mkdir(char *pathname, int mode);
int rmdir(char *pathname);
int link(char *oldname, char *newname);
int unlink(char *filename);
int mount(char *devname, char *dirname, int flags);
int umount(char *target);
int mknod(char *filename, int mode, int dev);
time_t time();
mode_t umask(mode_t mask);
void clear();
int stat(char *filename, stat_t *statbuf);
int fstat(fd_t fd, stat_t *statbuf);

int mkfs(char *devname, int icount);

#endif // OAK_SYSCALL_H
