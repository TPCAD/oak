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
