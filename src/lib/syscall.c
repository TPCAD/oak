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
 *  @brief  4 号系统调用
 *
 *  向文件写入字符串
 */
i32 write(fd_t fd, char *buf, u32 len) {
    return _syscall3(SYS_NR_WRITE, fd, (u32)buf, len);
}

i32 brk(void *addr) { return _syscall1(SYS_NR_BRK, (u32)addr); }
