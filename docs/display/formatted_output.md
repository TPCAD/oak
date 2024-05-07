# Formatted Output

## macros

```c
// %0
#define ZEROPAD 1  // 0x00000001 padding with 0
// %d, %i
#define SIGN 2     // 0x00000010 display sign
// %+
#define PLUS 4     // 0x00000100 force display plus sign
// default padding
#define SPACE 8    // 0x00010000 padding space
// %-
#define LEFT 16    // 0x00100000 left alignment
// %#
#define SPECIAL 32 // 0x01000000 display 0x
// %#x
#define SMALL 64   // 0x10000000 lowercase
```

## number

```c
static char *number(char *str, unsigned long num, int base, int size,
                    int precision, int flags) {
    // padding: ` ` or `0` for padding
    // sign: `-` or `+` for signed number
    // tmp: temporary buffer
    char padding, sign, tmp[36];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    int index;
    char *ptr = str;

    if (flags & SMALL) {
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }

    // can't pad number with 0 if left alignment
    if (flags & LEFT) {
        flags &= ~ZEROPAD;
    }

    if (base < 2 || base > 36) {
        return 0;
    }

    padding = (flags & ZEROPAD) ? '0' : ' ';

    if (flags & SIGN && num < 0) {
        sign = '-';
        num = -num;
    } else {
        sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);
    }

    if (sign) {
        size--;
    }

    if (flags & SPECIAL) {
        if (base == 16) {
            size -= 2;
        } else if (base == 8) {
            size--;
        }
    }

    // i: 数字转换后的字符个数
    i = 0;
    if (num == 0) {
        tmp[i++] = '0';
    } else {
        // the conversion result is reversed
        // i.e 33 is 0x21 in hexadecimal, the result is 12
        while (num != 0) {
            index = num % base;
            num /= base;
            tmp[i++] = digits[index];
        }
    }

    // 下面开始真正将数字写入到输出字符串

    // 对于整数，精度指定其最小个数
    if (i > precision) {
        precision = i;
    }
    // 减去精度，剩下即是需要填充的
    size -= precision;

    // 非 0 填充，非左对齐，使用空格填充
    if (!(flags & (ZEROPAD + LEFT))) {
        while (size-- > 0) {
            *str++ = ' ';
        }
    }

    // 写入符号
    if (sign) {
        *str++ = sign;
    }

    // 写入进制特殊符号
    if (flags & SPECIAL) {
        if (base == 8) {
            *str++ = '0';
        } else if (base == 16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }

    // 非左对齐，0 填充
    if (!(flags & LEFT)) {
        while (size-- > 0) {
            *str++ = padding;
        }
    }

    // 精度填充
    while (i < precision--) {
        *str++ = '0';
    }

    // 逆序写入转换后的字符
    while (i-- > 0) {
        *str++ = tmp[i];
    }

    // 左对齐，填充空格
    while ((size-- > 0)) {
        *str++ = ' ';
    }
    return str;
}
```
