# LD

## Linker Script

Linker Script 以 `.ld` 或 `.lds` 结尾，用于指示链接器如何进行链接。

### 入口地址

在链接脚本中可以使用 `ENTRY(symbol)` 指定入口地址。

```ld
ENTRY(_start)
```

### SECTIONS

SECTIONS 命令告诉链接器如何组织输入的各个段 以及如何在内存中排列输出的段。

```ld
SECTIONS {
    sections-command
    sections-command
    ...
}
```

### Location counter

`.` 是一个特殊的链接器变量，它表示当前位置离起始位置的字节偏移量。通常情况下，起始位置，也就是 `SECTIONS` 的地址是 0，因此，此时的 `.` 是绝对地址。当 `.` 出现在段描述中时，它表示当前位置离当前所在的段的字节偏移量。

```ld
SECTIONS {
    . = 0x10000; /* 绝对地址 */
    .text {      /* 将 .text 置于 0x10000 */
        *(.text)
        /* 相对地址，起始地址当前段的地址，即 0x10000，*/
        /* 实际地址则是 0x10040 */
        . = 0x40;
        *(.test)
    }
}
```

### 内建函数

#### FILL

填充间隙。

```ld
. = 0x10000;
FILL(0x00); /* 用 0x00 填充 0x40 字节
. = 0x10040;
```

## 参考文献

[LD document](https://sourceware.org/binutils/docs-2.39/ld.html)
