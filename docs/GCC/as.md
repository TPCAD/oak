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

### 标签

标签的作用相当于 C 中的函数名或变量名。

```assembly
# 定义一个名为 `message` 的变量
message:
    .asciz "Hello World!"

# 定义名为 `_add` 的函数
_add:
    movl 4(%esp), %eax
    addl 8(%esp), %eax
    ret

# 调用函数
call _add
```

标签的本质是地址。`call _add` 其实就是跳转到 `_add` 所代表的地址。

如果想获得标签的地址，可以使用 `$`。如 `movl $_add, %eax` 表示将 `_add` 所代表的地址加载到 `%eax`。这类似 C 中的「取地址」操作，`&_add`。

某些情况下 `$label` 和 `label` 的作用是一样的，如 `.long _add` 和 `.long $_add` 都表示将 `_add` 代表的地址存储在当前内存位置。

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

在 16 位模式下，`si` 的默认段寄存器是 `ds`，`di` 则是 `es`。

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

#### jmp

```asm
jmp loc
```

Jump，无条件跳转执行指定地址的代码，其实质是修改 `ip` 寄存器。

#### ljmp

```asm
ljmp seg, loc
```

Long Jump，无条件跳转执行指定地址的代码，同时修改 `cs` 和 `ip` 寄存器。

#### lods*

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

#### movs*

Move String，从 `ds:si` 拷贝一个单位数据到 `es:di`。

拷贝完成后 `si` 和 `di` 会根据 EFLAGS 的 DF 位加 1 或减 1。如果 DF 为 0 则加 1，为 1 则减 1。DF 位可以使用 `cld` 置为 0，使用 `std` 置为 1。

```assembly
.code16
    movw $0x07c0, %ax         # 将 ds 寄存器设为 0x07c0，用作源地址
    movw %ax, %ds
    movw $0x9000, %ax         # 将 es 寄存器设为 0x9000，用作目的地址
    movw %ax, %es
    movw $256, %cx            # 将 cx 寄存器设为 256，用作循环计数
    subw %si, %si             # 清空 si，di 寄存器
    subw %di, %di
    cld                       # 清空 Directive Flag，表示字符串操作时地址自增
    rep movsw                 # 每次从 ds:si 拷贝两字节到 es:si，重复 cx 次
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

### 逻辑运算符

GNU AS 的 `|` 和 `&` 是同一优先级的运算符，按从左到右的顺序运算。而 NASM 的 `&` 优先级要高于 `|`。

## 内联汇编

GNU 允许在 C 中嵌入汇编代码并读写 C 变量。

```language
asm asm-qualifiers ( AssemblerTemplate
                      : OutputOperands
                      : InputOperands
                      : Clobbers
                      : GotoLabels)
```

## 命令行参数

### -g

增加调试信息。

#### --32

编译 32 位程序。

## 参考文献

[AS Document](https://sourceware.org/binutils/docs/as/index.html)
[x86-gnu-assembly-primer.md - Github Gist](https://gist.github.com/AVGP/85037b51856dc7ebc0127a63d6a601fa)
[What is the purpose of GNU assembler directive .code16? - stackoverflow](https://stackoverflow.com/questions/60025609/what-is-the-purpose-of-gnu-assembler-directive-code16)
