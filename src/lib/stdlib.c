#include <oak/stdlib.h>
#include <oak/string.h>

/**
 *  @brief  将数字转换为字符串（十进制）
 *  @param  input  待转换的数字
 *  @param  buffer  存放转换后的字符串
 *
 * 数字对 10 求余可得其个位数字，加上字符 0 可得对应数字的 ASCII 码。
 * 数字除以 10 可得其除个位以外的数字。如此循环直至数字变为 0。
 * 最后得到的字符串是逆序的。
 * 以 1024 为例：
 * 1024 % 10 = 4 -> buffer[0] = 4
 * 1024 / 10 = 102
 * 102 % 10 = 2 -> buffer[1] = 2
 * 102 / 10 = 10
 * 10 % 10 = 0 -> buffer[2] = 0
 * 10 / 10 = 1
 * 1 % 10 = 1 -> buffer[3] = 1
 * 1 / 10 = 0
 */
void itoa(int input, char *buffer) {
    // 确定符号
    int sign = 0;
    if ((sign = input) < 0) {
        input = -input;
    }

    // 获取字符串（逆序）
    int str_len = 0;
    do {
        buffer[str_len++] = input % 10 + '0';
    } while ((input /= 10) > 0);

    // 写入符号
    if (sign < 0) {
        buffer[str_len++] = '-';
    }

    // 写入结束符
    buffer[str_len] = '\0';

    // 反转字符串
    reverse(buffer);
}
