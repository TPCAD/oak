# Make

## Makefile 语法

使用 **缩进** 而非空格

### 规则

```makefile
target ... : prerequisites ...
    recipe
    ...
```

- target: 文件或标签（伪指令）
- prerequisites: 生成 target 所需的前置文件或 target
- recipe: 生成 target 的一系列命令

### 伪指令

有时候 target 并不需要生成一个文件，而只是单纯地执行命令。

```makefile
.PHONY: clean
clean:
    rm -rf build
```
