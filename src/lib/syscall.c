#include <oak/syscall.h>

/**
 *  @brief  没有参数的系统调用
 *  @param  nr  系统调用号
 *  @return  系统调用返回值
 */
static u32 _syscall0(u32 nr) {
    u32 ret;
    asm volatile("int $0x80\n" : "=a"(ret) : "a"(nr));
    return ret;
}

/**
 *  @brief  有一个参数的系统调用
 *  @param  nr  系统调用号
 *  @param  arg  系统调用参数
 *  @return  系统调用返回值
 */
static u32 _syscall1(u32 nr, u32 arg) {
    u32 ret;
    asm volatile("int $0x80\n" : "=a"(ret) : "a"(nr), "b"(arg));
    return ret;
}

/**
 *  @brief  有两个参数的系统调用
 *  @param  nr  系统调用号
 *  @param  arg  系统调用参数
 *  @return  系统调用返回值
 */
static u32 _syscall2(u32 nr, u32 arg1, u32 arg2) {
    u32 ret;
    asm volatile("int $0x80\n" : "=a"(ret) : "a"(nr), "b"(arg1), "c"(arg2));
    return ret;
}

/**
 *  @brief  有三个参数的系统调用
 *  @param  nr  系统调用号
 *  @param  arg  系统调用参数
 *  @return  系统调用返回值
 */
static u32 _syscall3(u32 nr, u32 arg1, u32 arg2, u32 arg3) {
    u32 ret;
    asm volatile("int $0x80\n"
                 : "=a"(ret)
                 : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

/**
 *  @brief  0 号系统调用
 *  @return  系统调用返回值
 *
 *  用于测试系统调用
 */
u32 test() { return _syscall0(SYS_NR_TEST); }

/**
 *  @brief  162 号系统调用
 *
 *  任务主动进行调度
 */
void yield() { _syscall0(SYS_NR_YIELD); }

/**
 *  @brief  158 号系统调用
 *
 *  当前任务睡眠
 */
void sleep(u32 ms) { _syscall1(SYS_NR_SLEEP, ms); }

/**
 *  @brief  3 号系统调用
 *
 *  向文件写入字符串
 */
int read(fd_t fd, char *buf, int len) {
    return _syscall3(SYS_NR_READ, fd, (u32)buf, len);
}

/**
 *  @brief  4 号系统调用
 *
 *  向文件写入字符串
 */
int write(fd_t fd, char *buf, int len) {
    return _syscall3(SYS_NR_WRITE, fd, (u32)buf, len);
}

/**
 *  @brief  45 号系统调用
 *
 *  修改段地址
 */
i32 brk(void *addr) { return _syscall1(SYS_NR_BRK, (u32)addr); }

/**
 *  @brief  20 号系统调用
 *
 *  获取当前任务 ID
 */
pid_t getpid() { return _syscall0(SYS_NR_GETPID); }

/**
 *  @brief  64 号系统调用
 *
 *  获取当前任务的父任务 ID
 */
pid_t getppid() { return _syscall0(SYS_NR_GETPPID); }

/**
 *  @brief  2 号系统调用
 *
 *  创建子进程
 */
pid_t fork() { return _syscall0(SYS_NR_FORK); }

/**
 *  @brief  1 号系统调用
 *
 *  退出进程
 */
void exit(int status) { _syscall1(SYS_NR_EXIT, (u32)status); }

/**
 *  @brief  7 号系统调用
 *
 *  等待子进程退出
 */
pid_t waitpid(pid_t pid, i32 *status) {
    return _syscall2(SYS_NR_WAITPID, pid, (u32)status);
}

/**
 *  @brief  13 号系统调用
 *
 *  获取当前时间
 */
time_t time() { return _syscall0(SYS_NR_TIME); }

/**
 *  @brief  60 号系统调用
 *
 *  获取文件权限掩码
 */
mode_t umask(mode_t mask) { return _syscall1(SYS_NR_UMASK, (u32)mask); }

/**
 *  @brief  39 号系统调用
 *
 *  创建目录
 */
int mkdir(char *pathname, int mode) {
    return _syscall2(SYS_NR_MKDIR, (u32)pathname, (u32)mode);
}

/**
 *  @brief  40 号系统调用
 *
 *  删除目录
 */
int rmdir(char *pathname) { return _syscall1(SYS_NR_RMDIR, (u32)pathname); }

/**
 *  @brief  9 号系统调用
 *
 *  创建链接
 */
int link(char *oldname, char *newname) {
    return _syscall2(SYS_NR_LINK, (u32)oldname, (u32)newname);
}

/**
 *  @brief  10 号系统调用
 *
 *  删除链接
 */
int unlink(char *filename) { return _syscall1(SYS_NR_UNLINK, (u32)filename); }

/**
 *  @brief  5 号系统调用
 *
 *  打开文件
 */
fd_t open(char *filename, int flags, int mode) {
    return _syscall3(SYS_NR_OPEN, (u32)filename, (u32)flags, (u32)mode);
}

/**
 *  @brief  8 号系统调用
 *
 *  创建并打开文件
 */
fd_t create(char *filename, int mode) {
    return _syscall2(SYS_NR_CREAT, (u32)filename, (u32)mode);
}

/**
 *  @brief  6 号系统调用
 *
 *  关闭文件
 */
void close(fd_t fd) { _syscall1(SYS_NR_CLOSE, (u32)fd); }

/**
 *  @brief  19 号系统调用
 *
 *  设置文件偏移位置
 */
int lseek(fd_t fd, i32 offset, int whence) {
    return _syscall3(SYS_NR_LSEEK, fd, offset, whence);
}

/**
 *  @brief  183 号系统调用
 *
 *  获取当前工作目录
 */
char *getcwd(char *buf, size_t size) {
    return (char *)_syscall2(SYS_NR_GETCWD, (u32)buf, (u32)size);
}

/**
 *  @brief  12 号系统调用
 *
 *  修改进程工作目录
 */
int chdir(char *pathname) { return _syscall1(SYS_NR_CHDIR, (u32)pathname); }

/**
 *  @brief  19 号系统调用
 *
 *  修改进程根目录
 */
int chroot(char *pathname) { return _syscall1(SYS_NR_CHROOT, (u32)pathname); }

/**
 *  @brief  89 号系统调用
 *
 *  读取目录
 */
int readdir(fd_t fd, void *dir, int count) {
    return _syscall3(SYS_NR_READDIR, fd, (u32)dir, (u32)count);
}

/**
 *  @brief  200 号系统调用
 *
 *  清空屏幕
 */
void clear() { _syscall0(SYS_NR_CLEAR); }

/**
 *  @brief  18 号系统调用
 *
 *  文件状态
 */
int stat(char *filename, stat_t *statbuf) {
    return _syscall2(SYS_NR_STAT, (u32)filename, (u32)statbuf);
}

/**
 *  @brief  28 号系统调用
 *
 *  文件状态
 */
int fstat(fd_t fd, stat_t *statbuf) {
    return _syscall2(SYS_NR_FSTAT, (u32)fd, (u32)statbuf);
}
