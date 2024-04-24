
### CFI directives

> CFI stands for call frame information. It's the way the compiler describes
> what happens in a function. It can be used by the debugger to present a call
> stack, by the linker to synthesise exceptions tables, for stack depth analysis
> and other things like that.

CFI 指令以 `.cfi` 开头，是一种 `DWARF` 信息，用于调试。可以使用 gcc 编译参数
`fno-asynchronous-unwind-tables` 关闭。

https://stackoverflow.com/questions/15284947/understanding-gcc-s-output
https://stackoverflow.com/questions/24462106/what-do-the-cfi-directives-mean-and-some-more-questions
https://stackoverflow.com/questions/47026338/is-there-a-simple-dwarf-cfi-represenation-for-functions-that-set-up-a-convention
https://sourceware.org/binutils/docs/as/CFI-directives.html

### PIC

```s
call	__x86.get_pc_thunk.ax
```

使用 `-fno-pic` 关闭

### ident

```s
.ident	"GCC: (GNU) 13.2.1 20230801"
```
gcc 版本信息

使用 `-Qn` 关闭

### 栈对齐

```s
andl	$-16, %esp
```

将栈对齐到 16 字节。

使用 `-mpreferred-stack-boundary=2` 关闭

### 栈帧

```s
pushl	%ebp
movl	%esp, %ebp
...
leave
# popl %ebp
# movl %ebp, %esp
```

使用 `-fomit-frame-pointer` 关闭

### gcc 汇编代码分析

```s
	.file	"types.c"

	.text
	.globl	message
	.data
	.align 4
	.type	message, @object
	.size	message, 16
message:
	.string	"hello world!!!\n"

	.globl	buf
	.bss
	.align 32
	.type	buf, @object
	.size	buf, 1024
buf:
	.zero	1024

	.text
	.globl	main
	.type	main, @function
main:
	pushl	$message
	call	printf
	addl	$4, %esp
	movl	$0, %eax
	ret
	.size	main, .-main
	.section	.note.GNU-stack,"",@progbits # 标记栈不可运行
```

## 堆栈与函数

### 栈

栈是一种后进先出的数据结构，栈底位于高地址，栈顶向下增长，栈顶指针保存在
`ss:esp` 中。

`pusha` 以 `eax`，`ecx`，`edx`，`ebx`，`esp`，`ebp`，`esi`，`edi` 的顺序将 8 个
基础寄存器的值入栈，`popa` 将除 `esp` 外的 7 个基础寄存器恢复。

### 函数

- `call` 将下一条指令的地址压入栈中
- `ret` 将栈顶弹出到 `eip`
- `call` 与 `ret` 无关

### 局部变量与生命周期

```c
int add(int x, int y)
{
  int z = x + y;
  return z;
}

int main() {
  int a = 5;
  int b = 3;
  int c = add(a,b);
  return 0;
}
```


```s
	.file	"types.c"
	.text
	.globl	add
	.type	add, @function
add:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	movl	8(%ebp), %edx
	movl	12(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	leave
	ret
	.size	add, .-add
	.globl	main
	.type	main, @function
main:
    # 保存栈帧
	pushl	%ebp
	movl	%esp, %ebp
    # 分配空间以保存局部变量
    # 因为栈向下增长，所见减 12 实际上是增加 12 字节的空间
	subl	$12, %esp
    # 将局部变量 a、b 保存至栈中
	movl	$5, -12(%ebp)
	movl	$3, -8(%ebp)
    # 调用函数前将参数压入栈
	pushl	-8(%ebp)
	pushl	-12(%ebp)
    # 调用函数
	call	add
    # 调用结束，清除参数
	addl	$8, %esp
    # 将函数返回值压入之前分配的栈空间
	movl	%eax, -4(%ebp)
    # 寄存器传参
	movl	$0, %eax
    # 恢复栈帧
	leave
	ret
	.size	main, .-main
	.section	.note.GNU-stack,"",@progbits
```
