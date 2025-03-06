#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/string.h>
#include <oak/types.h>

// 以比特位记录 flags
#define ZEROPAD 1  // 0x00000001 以 0 填充
#define SIGN 2     // 0x00000010 展示符号
#define PLUS 4     // 0x00000100 强制显示 `+`
#define SPACE 8    // 0x00010000 以 ` ` 替代符号（不显示符号时）
#define LEFT 16    // 0x00100000 左对齐，默认右对齐
#define SPECIAL 32 // 0x01000000 十六进制数显示 `0x`
#define SMALL 64   // 0x10000000 小写

/**
 *  @brief  检查一个字符是否是数字
 *  @param  c  要检查的字符
 */
#define is_digit(c) ((c) >= '0' && (c) <= '9')

/**
 *  @brief  转换字符数字为数字
 *  @param  s  指向包含数字的字符串的指针
 *  @return  转换后的数字
 */
static int skip_atoi(const char **s) {
    int i = 0;

    while (is_digit(**s)) {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

/**
 *  @brief  将数字转换为字符串
 *  @param  str  buffer
 *  @param  num  要转换的数字
 *  @param  base  进制
 *  @param  field_width  字段宽度
 *  @param  precision  精度
 *  @param  flags  标志
 *  @return 新的 `str`
 */
static char *format_digit(char *str, unsigned long num, int base,
                          int field_width, int precision, int flags) {
    const char *digits = "0123456789ABCDEFX";

    if (flags & SMALL) {
        digits = "0123456789abcdefx";
    }

    // 数字左对齐不允许用 0 填充
    if (flags & LEFT) {
        flags &= ~ZEROPAD;
    }

    // TODO: assert base
    if (base < 2 || base > 36) {
        return 0;
    }

    /*  先计算转换结果的长度。因为当被转换值和精度都为 0 时，转换结果无字符。
     *  这种情况下，字段宽度全部用空格字符填充，符号，替用形式都不起作用。*/

    // 转换数字为字符串，结果暂存与 `tmp`，`str_len` 记录结果字符串长度
    int str_len = 0;
    char tmp[36];
    if (num == 0) {
        tmp[str_len++] = '0';
    } else {
        while (num != 0) {
            int idx = num % base;
            num /= base;
            tmp[str_len++] = digits[idx];
        }
    }

    // 负整数精度不起作用
    precision = (precision < 0) ? 1 : precision;

    // 被转换值和精度都为 0，则转换结果无字符
    if (precision == 0 && str_len == 1 && tmp[0] == '0') {
        str_len = 0;
    }

    // 精度指定出现数字的最小个数，若小于字符串长度或为零则忽略
    field_width -= (precision > str_len ? precision : str_len);

    // 填充字符
    char padding_char = (flags & ZEROPAD) ? '0' : ' ';

    // 符号字符，`+`，`-`，` ` 或无
    char sign_char = EOS;
    if (flags & SIGN && num < 0) {
        sign_char = '-';
        num = -num;
    } else {
        sign_char = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);
    }
    if (sign_char && str_len != 0 && (base != 8 || base != 16)) {
        field_width--;
    }

    // 替用形式，八进制数前缀 `0`，十六进制数前缀 `0x` 或 `0X`
    if ((flags & SPECIAL) && str_len > 0) {
        if (base == 16) {
            field_width -= 2;
        } else if (base == 8) {
            field_width--;
        }
    }

    // 右对齐，非 0 填充，使用 ` ` 填充
    // 0 填充，但指定了精度，则忽略 0 标志，使用 ` ` 填充
    if (!(flags & (ZEROPAD + LEFT)) || ((flags & ZEROPAD) && precision > 1)) {
        while (field_width-- > 0) {
            *str++ = ' ';
        }
    }

    // 写入符号
    if (sign_char && str_len > 0 && (base != 8 || base != 16)) {
        *str++ = sign_char;
    }

    // 写入替用形式
    if ((flags & SPECIAL) && str_len > 0) {
        if (base == 8) {
            *str++ = '0';
        } else if (base == 16) {
            *str++ = '0';
            *str++ = digits[16];
        }
    }

    // 右对齐，写入填充字符
    if (!(flags & LEFT)) {
        while (field_width-- > 0) {
            *str++ = padding_char;
        }
    }

    // 写入精度填充字符
    while (str_len < precision--) {
        *str++ = '0';
    }

    // 写入数字
    while (str_len-- > 0) {
        *str++ = tmp[str_len];
    }

    // 左对齐，写入填充字符
    while (field_width-- > 0) {
        *str++ = ' ';
    }

    return str;
}

/**
 *  @brief  格式化字符串并将结果写入目标字符串
 *  @param  buf  指向要写入的字符串的指针
 *  @param  fmt  指向格式字符串的指针
 *  @param  vlist  包含要打印数据的变量参数列表
 *  @return  写入到 buf 的字符数，不包括空终止符
 *
 *  格式化字符串以 `%` 开头，以指示符结尾。
 *
 *  ```language
 *  %[flags][width][.precision][length]specifier
 *  ```
 */
int vsprintf(char *buf, const char *fmt, va_list vlist) {
    char *str = NULL;
    int flags = 0;
    int field_width = 0;
    int precision = 0;
    int qualifier = 0;

    char *s = NULL;
    int str_len = 0;
    int *ip = NULL; // 指向存储以写入的字符数的内存（见 %n）
    for (str = buf; *fmt; fmt++) {
        // 处理普通字符
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        // 处理 flags
        flags = 0;
    repeat:
        ++fmt; // 下一个 flags
        switch (*fmt) {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        // 处理字段宽度，整数或 `*`
        field_width = -1;

        if (is_digit(*fmt)) { // 整数
            field_width = skip_atoi(&fmt);
        } else if (*fmt == '*') { // `*`
            ++fmt;
            field_width = va_arg(vlist, int);

            // 负数实参导致 `-` 标志和正字段宽度
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        // 处理精度，整数或 `*`
        precision = -1;

        if (*fmt == '.') {
            ++fmt;
            if (is_digit(*fmt)) { // 整数
                precision = skip_atoi(&fmt);
            } else if (*fmt == '*') { // `*`
                precision = va_arg(vlist, int);
            } else {
                precision = 0;
            }

            // 负整数导致精度失效，处理转换格式指示符时会识别并处理负整数精度
        }

        // 处理长度。只支持以下三种：
        // h: short, l: long, L: long long
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            ++fmt;
        }

        switch (*fmt) {
        case 'c': // 字符，flags, precision 对 c 无效
            // 右对齐
            if (!(flags & LEFT)) {
                while (--field_width > 0) {
                    *str++ = ' ';
                }
            }
            *str++ = (unsigned char)va_arg(vlist, int);

            // 左对齐
            while (--field_width > 0) {
                *str++ = ' ';
            }
            break;
        case 's': // 字符串，flags 不起作用（除了 `-`）
            s = va_arg(vlist, char *);
            str_len = strlen(s);

            // 精度决定写入的长度
            if (precision < 0) {
                precision = str_len;
            } else if (str_len > precision) {
                str_len = precision;
            }

            // 右对齐
            if (!(flags & LEFT)) {
                while (str_len < field_width--) {
                    *str++ = ' ';
                }
            }

            // 写入字符
            for (int i = 0; i < str_len; i++) {
                *str++ = *s++;
            }

            // 左对齐
            while (--field_width > str_len) {
                *str++ = ' ';
            }
            break;
        case 'o':
            str = format_digit(str, va_arg(vlist, unsigned long), 8,
                               field_width, precision, flags);
            break;
        case 'p':
            if (field_width == -1) {
                field_width = 8;
                flags |= ZEROPAD;
            }
            str = format_digit(str, (unsigned long)va_arg(vlist, void *), 16,
                               field_width, precision, flags);
            break;
        case 'x':
            flags |= SMALL;
        case 'X':
            str = format_digit(str, va_arg(vlist, unsigned long), 16,
                               field_width, precision, flags);
            break;
        case 'i':
        case 'd':
            flags |= SIGN;
        case 'u':
            str = format_digit(str, va_arg(vlist, unsigned long), 10,
                               field_width, precision, flags);
            break;
        case 'n':
            ip = va_arg(vlist, int *);
            *ip = (str - buf);
            break;
        default:
            if (*fmt == '%') {
                *str++ = '%';
            }
            if (*fmt) {
                *str++ = *fmt;
            } else {
                --fmt;
            }
            break;
        }
    }
    *str = '\0';

    int tmp = str - buf;
    // TODO: assert tmp
    return tmp;
}

/**
 *  @brief  格式化字符串并将结果写入目标字符串
 *  @param  buf  指向要写入的字符串的指针
 *  @param  fmt  指向格式字符串的指针
 *  @param  ...  格式化参数
 *  @return  写入到 buf 的字符数，不包括空终止符
 */
int sprintf(char *buf, const char *fmt, ...) {
    va_list vlist = NULL;
    va_start(vlist, fmt);
    int i = vsprintf(buf, fmt, vlist);
    va_end(vlist);
    return i;
}
