# Variadic Parameters

拥有可变参数的函数的第一个参数应该是可变参数的个数。

```c
test(int cnt, ...);

// 第一个参数 4 表示后面还有 4 个参数
test(4, 33, 12, 43, 88);
```

```c
typedef char *va_list;

#define va_start(ap, v) (ap = (va_list) & v + sizeof(char *))
#define va_arg(ap, t) (*(t *)((ap += sizeof(char *)) - sizeof(char *)))
#define va_end(ap) (ap = (va_list)0)
```

## va_start

```c
#define va_start(ap, v) (ap = (va_list) & v + sizeof(char *))
```

`ap` 是指向可变参数的指针，初始化为 0。`v` 是函数的第一个参数。

宏 `va_start` 把指针 `ap` 指向第一个可变参数。取 `v` 的地址并将其加上一个指针长度得到第一个可变参数的地址。

## va_arg

```c
#define va_arg(ap, t) (*(t *)((ap += sizeof(char *)) - sizeof(char *)))
```

`ap` 是指向可变参数的指针，初始化为 0。`t` 是可变参数的数据类型。

宏 `va_arg` 把指针 `ap` 指向下一个可变参数，并返回当前可变参数的值。

```c
(ap += sizeof(char *))
```

该表达式将 `ap` 移动，并将移动后的值返回。

```c
((ap += sizeof(char *)) - sizeof(char *))
```

该表达式将 `ap` 移动，并返回 `ap` 的原来的值。