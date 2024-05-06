#include <oak/stdarg.h>
#include <oak/stdio.h>
#include <oak/string.h>

#define ZEROPAD 1  // 0x00000001 padding with 0
#define SIGN 2     // 0x00000010 display ???
#define PLUS 4     // 0x00000100 force display sign
#define SPACE 8    // 0x00010000 padding space
#define LEFT 16    // 0x00100000 left alignment
#define SPECIAL 32 // 0x01000000 display 0x
#define SMALL 64   // 0x10000000 lowercase

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s) {
    int i = 0;

    while (is_digit(**s)) {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

// str: the output string
// num: variadic parameters in printk
// base: base number
// size: length of string
// precision: precision
// flags:
static char *number(char *str, unsigned long num, int base, int size,
                    int precision, int flags) {
    char c, sign, tmp[36];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    int index;
    char *ptr = str;

    if (flags & SMALL) {
        digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    }

    if (flags & LEFT) {
        flags &= ~ZEROPAD;
    }

    if (base < 2 || base > 36) {
        return 0;
    }

    c = (flags & ZEROPAD) ? '0' : ' ';

    if (flags & SIGN && num < 0) {
        sign = '-';
        num = -num;
    } else {
        sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : '0');
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

    // conversion
    i = 0;
    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            index = num % base;
            num /= base;
            tmp[i++] = digits[index];
        }
    }

    if (i > precision) {
        precision = i;
    }
    sign -= precision;

    if (!(flags & (ZEROPAD + LEFT))) {
        while (size-- > 0) {
            *str++ = ' ';
        }
    }

    if (sign) {
        *str++ = sign;
    }

    if (flags & SPECIAL) {
        if (base == 8) {
            *str++ = '0';
        } else if (base == 16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }

    if (!(flags & LEFT)) {
        while (size-- > 0) {
            *str++ = c;
        }
    }

    while (i < precision--) {
        *str++ = '0';
    }

    while (i-- > 0) {
        *str++ = tmp[i];
    }

    while ((size-- > 0)) {
        *str++ = ' ';
    }
    return str;
}

// %[flags][width][.precision][length]specifier
int vsprintf(char *buf, const char *fmt, va_list args) {

    // handle %s
    int len;
    char *str;
    char *s;

    int *ip;

    int flags = 0; // flags

    int field_width; // width
    int precision;   // precision
    int qualifier;   // length, h, l, L for intergers

    // traverse strings in fmt
    for (str = buf; *fmt; ++fmt) {
        // write non-formatted characters to `str`
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        // handle flags
        // including `-`, `+`, ` `, `#`, `0`
        flags = 0;
    repeat:
        ++fmt; // skip %
        switch (*fmt) {
            // left alignment
        case '-':
            flags |= LEFT;
            goto repeat;
            // force display sign
        case '+':
            flags |= PLUS;
            goto repeat;
            // space padding
        case ' ':
            flags |= SPACE;
            goto repeat;
            // special conversion
        case '#':
            flags |= SPECIAL;
            goto repeat;
            // zero padding
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        // handle width
        // including `*`, interger
        field_width = -1;

        if (is_digit(*fmt)) {
            field_width = skip_atoi(&fmt);
        } else if (*fmt == '*') {
            ++fmt;
            field_width = va_arg(args, int);

            // it means that the parameter contains a flag `-` if field_width
            // less than 0
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        // handle precision
        // precison starts with `.`
        precision = -1;

        if (*fmt == '.') {
            ++fmt;
            if (is_digit(*fmt)) {
                precision = skip_atoi(&fmt);
            } else if (*fmt == '*') {
                precision = va_arg(args, int);
            }

            // ignore `-`
            precision = precision < 0 ? -precision : precision;
            // if (precision < 0) {
            //     precision = -precision;
            // }
        }

        // handle length
        // including `h`, `l`, `L`
        qualifier = -1;

        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            ++fmt;
        }

        // handle specifier
        switch (*fmt) {
            // character
        case 'c':
            // right alignment
            if (!(flags & LEFT)) {
                while (--field_width > 0) {
                    *str++ = ' ';
                }
            }
            *str++ = (unsigned char)va_arg(args, int);

            // left alignment
            while (--field_width > 0) {
                *str++ = ' ';
            }
            break;
            // string
        case 's':
            s = va_arg(args, char *);
            len = strlen(s);

            if (precision < 0) {
                precision = len;
            } else if (len > precision) {
                len = precision;
            }

            if (!(flags & LEFT)) {
                while (len < field_width--) {
                    *str++ = ' ';
                }
            }

            for (int i = 0; i < len; i++) {
                *str++ = *s++;
            }

            // left alignment
            while (--field_width > 0) {
                *str++ = ' ';
            }
            break;
            // octal integer
        case 'o':
            str = number(str, va_arg(args, unsigned long), 8, field_width,
                         precision, flags);
            break;
            // pointer
        case 'p':
            if (field_width == -1) {
                field_width = 8;
                flags |= ZEROPAD;
            }
            str = number(str, (unsigned long)va_arg(args, void *), 16,
                         field_width, precision, flags);
            break;
            // hexadeciaml
        case 'x':
            flags |= SMALL;
        case 'X':
            str = number(str, va_arg(args, unsigned long), 16, field_width,
                         precision, flags);
            break;
            // decimal
        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            str = number(str, va_arg(args, unsigned long), 10, field_width,
                         precision, flags);
            break;
        case 'n':
            ip = va_arg(args, int *);
            *ip = (str - buf);
            break;
        default:
            if (*fmt != '%') {
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

    return str - buf;
}
