#include <oak/syscall.h>
#include <oak/types.h>

static _inline u32 _syscall0(u32 nr) {
    u32 ret;
    asm volatile("int $0x80\n" : "=a"(ret) : "a"(nr));
    return ret;
}

static _inline u32 _syscall1(u32 nr, u32 arg) {
    u32 ret;
    asm volatile("int $0x80\n" : "=a"(ret) : "a"(nr), "b"(arg));
    return ret;
}

static _inline u32 _syscall2(u32 nr, u32 arg1, u32 arg2) {
    u32 ret;
    asm volatile("int $0x80\n" : "=a"(ret) : "a"(nr), "b"(arg1), "c"(arg2));
    return ret;
}

static _inline u32 _syscall3(u32 nr, u32 arg1, u32 arg2, u32 arg3) {
    u32 ret;
    asm volatile("int $0x80\n"
                 : "=a"(ret)
                 : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

u32 test() { return _syscall0(SYS_NR_TEST); }

void yield() { _syscall0(SYS_NR_YIELD); }

void sleep(u32 ms) { _syscall1(SYS_NR_SLEEP, ms); }

int32 write(fd_t fd, char *buf, u32 len) {
    return _syscall3(SYS_NR_WRITE, fd, (u32)buf, len);
}

int mkdir(char *pathname, int mode) {
    return _syscall2(SYS_NR_MKDIR, (u32)pathname, (u32)mode);
}

int rmdir(char *pathname) { return _syscall1(SYS_NR_RMDIR, (u32)pathname); }

int link(char *oldname, char *newname) {
    return _syscall2(SYS_NR_LINK, (u32)oldname, (u32)newname);
}
int unlink(char *filename) { return _syscall1(SYS_NR_UNLINK, (u32)filename); }

time_t time() { return _syscall0(SYS_NR_TIME); }

mode_t umask(mode_t umask) { return _syscall1(SYS_NR_UMASK, (u32)umask); }

int32 brk(void *addr) { return _syscall1(SYS_NR_BRK, (u32)addr); }

pid_t get_pid() { return _syscall0(SYS_NR_GETPID); }
pid_t get_ppid() { return _syscall0(SYS_NR_GETPPID); }
pid_t fork() { return _syscall0(SYS_NR_FORK); }
void exit(int status) { _syscall1(SYS_NR_EXIT, (u32)status); }
pid_t waitpid(pid_t pid, int32 *status) {
    return _syscall2(SYS_NR_WAITPID, pid, (u32)status);
}
