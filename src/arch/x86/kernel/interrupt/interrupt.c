#include <oak/interrupt/interrupt.h>

/**
 *  @brief  清除 IF 位并返回清除前的值
 *  @return  清除前 IF 的值
 */
bool diable_interrupt() {
    asm volatile("pushfl\n"
                 "cli\n"
                 "popl %eax\n"
                 "shrl $9, %eax\n"
                 "andl $1, %eax\n");
}

/**
 *  @brief  获取 IF 位的值
 *  @return  IF 的值
 */
bool get_interrupt_state() {
    asm volatile("pushfl\n"
                 "popl %eax\n"
                 "shrl $9, %eax\n"
                 "andl $1, %eax\n");
}

/**
 *  @brief  设置或清除 IF 位
 *  @param  bool
 */
void set_interrupt_state(bool state) {
    if (state) {
        asm volatile("sti\n");
    } else {
        asm volatile("cti\n");
    }
}
