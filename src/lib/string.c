#include <oak/string.h>

size_t strlen(const char *str) {
    const char *ptr = str;
    while (*ptr != EOS) {
        ptr++;
    }
    return ptr - str;
}

int strcmp(const char *lhs, const char *rhs) {
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS) {
        lhs++;
        rhs++;
    }
    return *lhs < *rhs ? -1 : 1;
}

char *strchr(const char *str, int ch) {
    do {
        if (*str == (char)ch) {
            return (char *)str;
        }
    } while (*str++);
    return NULL;
}

char *strrchr(const char *str, int ch) {
    char *ptr = NULL;
    do {
        if (*str == (char)ch) {
            ptr = (char *)str;
        }
    } while (*str++);
    return ptr;
}

char *strcpy(char *dest, const char *src) {
    size_t len = strlen(src);
    if (dest < src) {
        const unsigned char *firsts = (const unsigned char *)src;
        unsigned char *firstd = (unsigned char *)dest;
        while (len--) {
            *firstd++ = *firsts++;
        }
    } else {
        const unsigned char *lasts = (const unsigned char *)src + (len - 1);
        unsigned char *lastd = (unsigned char *)dest + (len - 1);
        while (len--) {
            *lastd-- = *lasts--;
        }
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *ptr = dest;
    while (*ptr != EOS) {
        ptr++;
    }

    while (*src != EOS) {
        *ptr = *src;
        ptr++;
        src++;
    }
    *ptr = EOS;

    return dest;
}

/**
 *  @brief  在 `ptr` 所指向对象的起始 `count` 个字节（均转译成 unsigned char）
 *          中寻找首次出现的 `(unsigned char)ch`。
 *
 *  @param  ptr  指向要检验对象的指针
 *  @param  ch  要检索的字节
 *  @param  count  要检验的最大字节数
 *
 *  @return  返回值为指向字节位置的指针，或若找不到该字节则为空指针。
 */
void *memchr(const void *ptr, int ch, size_t count) {
    const unsigned char *src = (const unsigned char *)ptr;

    while (count-- > 0) {
        if (*src == ch) {
            return (void *)src;
        }
        src++;
    }
    return NULL;
}

/**
 *  @brief  比较 `lhs` 和 `rhs` 所指向的对象的开头 `count` 字节。按字典序比较。
 *
 *  @param  lhs, rhs  指向要比较的对象的指针
 *  @param  count  要检验的最大字节数
 *
 *  @return  若 lhs 按字典序先于 rhs 出现，则为 -1。
 *           若 lhs 与 rhs 比较相等，或 count 为零则为 0。
 *           若 lhs 按字典序晚于 rhs 出现，则为 1。
 */
int memcmp(const void *lhs, const void *rhs, size_t count) {
    const unsigned char *s1 = (const unsigned char *)lhs;
    const unsigned char *s2 = (const unsigned char *)rhs;

    while (count-- > 0) {
        if (*s1 != *s2) {
            return (int)(*s1 < *s2 ? -1 : 1);
        }
        s1++;
        s2++;
    }
    return 0;
}

/**
 *  @brief  将值 `(unsigned char)ch` 复制到 `dest` 所指向对象的最前面 `count`
 *          个字节中。
 *
 *  @param  dest  指向要填充的对象的指针
 *  @param  ch  填充字节
 *  @param  count  要填充的字节数
 *
 *  @return  dest
 */
void *memset(void *dest, int ch, size_t count) {
    unsigned char *ptr = (unsigned char *)dest;

    while (count-- > 0) {
        *ptr++ = (unsigned char)ch;
    }
    return dest;
}

/**
 *  @brief  从 `src` 所指向的对象复制 `count` 个字符到 `dest` 所指向的对象
 *
 *  @param  dest  指向复制目标对象的指针
 *  @param  src  指向复制来源对象的指针
 *  @param  count  复制的字节数
 *
 *  @return  dest
 */
void *memcpy(void *dest, const void *src, size_t count) {
    if (dest < src) {
        const unsigned char *firsts = (const unsigned char *)src;
        unsigned char *firstd = (unsigned char *)dest;
        while (count--) {
            *firstd++ = *firsts++;
        }
    } else {
        const unsigned char *lasts = (const unsigned char *)src + (count - 1);
        unsigned char *lastd = (unsigned char *)dest + (count - 1);
        while (count--) {
            *lastd-- = *lasts--;
        }
    }
    return dest;
}

/**
 *  @brief  就地反转字符串
 *  @param  s  要反转的字符串
 */
void reverse(char *s) {
    for (int i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        char c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
