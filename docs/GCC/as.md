# GNU Assembly

## AT&T 汇编语法

### 格式

```language
mnemonic source, destination
```

```assembly
movb $0x12, %al
```

表示将值 `0x12` 写入到寄存器 `al`。

### 前缀

寄存器必须加上前缀 `%`，如 `%al`，`%ax`，`%si`。数字常量必须加上前缀 `$`，如 `$0x12`，`$12`。

### 后缀

指令通常带有指明操作数大小的后缀。

- `b`：byte（8 bit）
- `s`：single（32 bit 浮点型）
- `w`：word（16 bit）
- `l`：long（32 bit 整型或 64 bit 浮点型）
- `q`：quad（64 bit）
- `t`：ten bytes（80 bit 浮点型）

如果不带后缀，那么编译器会根据目标寄存器进行推断。

### 常用寄存器

#### 通用寄存器（General-Purpose Register）

通用寄存器可用于几乎所有用途，并不局限于下面提到的场景。

- `ax`：Accumulator register，累加寄存器，常用于算术运算
- `bx`：Base register，基址寄存器，常用于段模式寻址
- `cx`：Counter register，计数寄存器，常用于循环计数
- `sp`：Stack Pointer register，栈指针寄存器，指向栈顶
- `bp`：Stack Base Pointer register，栈底指针寄存器，指向栈底
- `di`：Destination Index register，目的索引寄存器，指向流操作中目的地址
- `si`：Source Index register，源索引寄存器，指向流操作中源地址
- `dx`：Data register，数据寄存器，常用于算术运算和 IO 操作

上面的顺序也是通用寄存器的入栈顺序。

#### 段寄存器（Segment Register）

- `ss`：Stack Segment，存放栈的起始地址
- `cs`：Code Segment，存放代码段的起始地址
- `ds`：Data Segment，存放数据段的起始地址
- `es`：Extra Segment，存放额外代码段的起始地址
- `fs`：F Segment，存放额外代码段的起始地址
- `gs`：G Segment，存放额外代码段的起始地址

#### EFLAGS 寄存器

TODO:

### 常用指令

以下指令将省略后缀。

#### lods

Load String，从内存地址 `ds:si` 中加载一个单位的数据到合适的寄存器（al、ax）。

加载完成后 `si` 会根据 EFLAGS 的 DF 位加 1 或减 1。如果 DF 为 0 则加 1，为 1 则减 1。DF 位可以使用 `cld` 置为 0，使用 `std` 置为 1。

```assembly
    movw $msg, %si
    movb $0xe, %ah
print_char:
    lodsb
    cmpb $0, %al
    je done
    int $0x10
    jmp print_char
done:
    hlt
```

### 汇编器指令

汇编器指令是用于提示编译器编译的指令，不是汇编指令。汇编器指令以 `.` 开头，并且区分大小写，通常都是小写。

#### .

指代当前位置。

```assembly
.fill 510-(.-_start), 1, 0
```

#### .ascii/.asciz

```language
.ascii "String1", "String2", ...
```

连续存储一系列没有结束符的字符串。

```language
.asciz "String1", "String2", ...
```

连续存储一系列有结束符的字符串。不按照逗号进行分隔的字符串会被合并到一起。

#### .globl/.global

```language
.globl symbol
.global symbol
```

使 `symbol` 对于 ld 可见。

#### .fill

```language
.fill repeat, size, value
.fill repeat
```

填充 `repeat` 次 `size` 字节的 `value`。`size` 最大为 8 字节。如果只有 `repeat` 一个参数，则 `size` 为 1，`value` 默认为 0。

#### Data Types

| 预处理器指令 | 字节数 | 含义        |
| ------------ | ------ | ----------- |
| .byte        | 1      | 预留 1 字节 |
| .word        | 2      | 预留 2 字节 |
| .long        | 4      | 预留 4 字节 |
| .quad        | 8      | 预留 8 字节 |
| .short       | 2      | 预留 2 字节 |
| .int         | 4      | 预留 4 字节 |

