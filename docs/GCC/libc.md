# libc

## string.h

### strlen

```c
size_t strlen( const char* str );
```

返回给定空终止字符串的长度，即首元素为 `str` 所指的字符数组中，直至且不包含首个空字符的字符数。

### strcmp

```c
int strcmp( const char* lhs, const char* rhs );
```

以字典序比较两个空终止字节字符串。与 `memcmp` 类似。

### strchr

```c
char* strchr( const char* str, int ch );
```

寻找 `ch`（如同用 `(char)ch` 转换成 `char` 后）在 `str` 所指向的空终止字节字符串（转译每个字符为 `unsigned char`）中的首次出现位置。终止空字符被认为是字符串的一部分，并且能在寻找 `\0` 时找到。

返回指向 `str` 找到的字符的指针，若未找到该字符则为空指针。

### strrchr

```c
char* strrchr( const char* str, int ch );
```

寻找 `ch`（如同用 `(char)ch` 转换到 `char` 后）在 `str` 所指向的空终止字节串中（将每个字符转译成 `unsigned char`）的最后出现位置。若搜索 `\0`，则认为终止空字符为字符串的一部分，而且能找到。

### strcpy

```c
char *strcpy( char *dest, const char *src );
```

复制 `src` 所指向的空终止字节字符串，包含空终止符，到首元素为 `dest` 所指的字符数组。

### strcat

```c
char *strcat( char *dest, const char *src );
```

将 `src` 所指向的空终止字节字符串的副本追加到 `dest` 所指向的空终止字节字符串的末尾。字符 `src[0]` 替换 `dest` 末尾的空终止符。产生的字节字符串是空终止的。

### memchr

```c
void *memchr(const void *ptr, int ch, size_t count);
```

在 `ptr` 所指向对象的起始 `count` 个字节（均转译成 unsigned char）中寻找首次出现的 `(unsigned char)ch`。

返回值为指向字节位置的指针，或若找不到该字节则为空指针。

```c
void *memchr(const void *ptr, int ch, size_t count) {
    const unsigned char *src = (const unsigned char *)ptr;

    while (count-- > 0) {
        if (*src == (unsigned char)ch) {
            return (void *)src;
        }
        src++;
    }
    return NULL;
}
```

### memcmp

```c
int memcmp(const void *lhs, const void *rhs, size_t count);
```

比较 `lhs` 和 `rhs` 所指向的对象的开头 `count` 字节。按字典序比较。

返回值是 `-1`、`0` 或 `1`，若 `lhs` 按字典序先于 `rhs` 出现，则为负值，反之则为正值。若相等或 `count` 为零则为零。

该函数读取 **对象表示**，而非对象值。因此需要先将 `lhs` 和 `rhs` 转换为 `unsigned char`。

```c
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
```

### memset

```c
void *memset(void *dest, int ch, size_t count);
```

将值 `(unsigned char)ch` 复制到 `dest` 所指向对象的最前面 `count` 个字节中。

### memcpy

```c
void* memcpy( void *dest, const void *src, size_t count );
```

从 `src` 所指向的对象复制 `count` 个字符到 `dest` 所指向的对象。

## stdarg.h

可变参数函数是使用可变数量实参的函数，如 `printf`。可变数量实参由 `...` 形式的形参所指定，它必须出现在形参列表的最后，并且其前面至少有一个具名形参。

```c
int printx(const char* fmt, ...);
```

在函数体内使用变长实参时，这些必须用 `<stdarg.h>` 库设施访问。

```c
typedef char *va_list;
#define va_start(ap, v) (ap = (va_list) & v + sizeof(char *))
#define va_arg(ap, t) (*(t *)((ap += sizeof(char *)) - sizeof(char *)))
#define va_end(ap) (ap = (va_list)0)
```

### va_start

```c
#define va_start(ap, v) (ap = (va_list) & v + sizeof(char *))
```

在 32 位 x86 机器上，C 使用栈进行参数传递，在调用函数前会先将参数 **从右到左** 压入栈中。此时栈顶指针指向的就是函数的第一个参数。

```language
int va_test(int cnt, ...);

+ -------- + <- esp
|    cnt   |
+ -------- +
|first varg|
+ -------- +
| ...      |
+ -------- +
```

`ap` 是用于指向可变实参的指针，使用前应初始化为空指针。`v` 是首个变长形参前的具名形参，也就是 `va_teat` 中的 `cnt`。

`va_start` 宏的作用是使 `ap` 指向第一个变长实参。`(va_list)&v` 将 `v` 的地址，也就是当前 `esp` 的值，转换成 `va_list` 类型，再加上一个指针的长度就得到了第一个变长实参的地址（栈从高地址向低地址增长）。

### va_arg

```c
#define va_arg(ap, t) (*(t *)((ap += sizeof(char *)) - sizeof(char *)))
```

`t` 是变长实参的类型。`va_arg` 宏的作用是返回当前变长实参的值，并将指针 `ap` 移动到下一个变长实参。

表达式 `ap += sizeof(char*)` 的返回值是一个临时值，该值与 `ap` 的新值相等，使用该临时值进行运算不会影响 `ap` 的值。因此，表达式 `((ap += sizeof(char *)) - sizeof(char *))` 的返回值就是第一个变长实参的地址，也就是 `ap` 的旧值。

### 示例

```c
int add(int cnt, ...){
    int result = 0;

    va_list args = NULL;
    va_start(args, cnt);
    while (cnt--) {
        result += va_arg(args, int);
    }
    va_end(args);

    return result;
}
```

## stdio.h

### vsprintf

格式字符串有普通多字节字符（除了 %）转换指示构成。每个转换指示均拥有如下格式：

```language
%[flags][width][.precision][length]specifier
```

#### flags

- `-`：转换在宽度内左对齐
- `+`：强制显示符号（默认只有负数显示符号）
- ` `：若不显示符号则以空格替代符号
- `#`：进行 **替用形式** 的转换
- `0`：对于整数或浮点数，使用 `0` 替换 ` ` 作为填充字符。对于整数，若指定了精度，忽略此标志。若存在 `-`，忽略此标志

#### width

正整数或 `*`，指定最小字段宽度。以填充字符填充多余宽度。

若为 `*`，需要一个额外的 `int` 实参指定宽度，如果实参是负数，将导致 `-` 标志和正的字段宽度。

字段宽度小于转换结果所需的长度时，字段宽度不起作用。

#### .precision

后随整数或 `*` 或两者皆无的 `.`，指示转换的精度。`*` 情况与 width 相同，但实参为负数则忽略。

若仅有 `.`，则精度为 0。

对于整数，精度指定数字出现的最小位数，若小于转换结果所需的长度，则不起作用。

#### length

与格式指示符组合指定对应实参的类型。

#### specifier

##### c

写入单个字符。实参首先被转换成 `unsigned char`。flags 和 precision 不起作用。

##### s

写入字符串。精度指定写入的最大字符数，若没有则写入到首个空终止符（不包含）。flags 不起作用（除了 `-`）。

##### d，i，o，x，X

`d`，`i`：转换有符号整数为十进制表示。
`o`：转换有符号整数为八进制表示。
`x`、`X`：转换有符号整数为十六进制表示。

精度指定数字出现的最小位数，若小于转换结果所需的长度，则不起作用。若精度和被转换值都为 0，那么转换结果无字符。

## 参考文献

[Strings library - cppreference](https://en.cppreference.com/w/c/string/byte)

