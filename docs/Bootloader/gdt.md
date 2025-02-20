# GDT

GDT（Global Descriptor Table）全局描述符表，是一个用于描述内存分布的数组，数组
的每一项称为段描述符（Segment Descriptor），用于描述一个内存区域。GDT 的第一项
必须为全 0。在进入保护模式之前要先将 GDT 加载到寄存器 `gdtr` 中。

## `gdtr` 寄存器

结构：

- 0 ～ 1 bytes：GDT 的大小
- 2 ～ 5 bytes：描述符的偏移地址，即在数组中的位置

```asm
lgdt <gdt_ptr> ; 加载 GDT 到 gdtr
sgdt <gdt_ptr> ; 保存 GDT
```

## 段描述符

<img src="https://habrastorage.org/r/w1560/storage1/c46ddde2/3ff7d2f3/6af3408f/0d65fa31.jpg">

- 基地址（base）：32 bits，表示内存段的基地址
- 内存界限（limit）：32 bits，表示内存段大小
- 访问类型（access type）：40～47 位，用于控制访问权限
- 标志（Flags）：52~55 位，控制粒度，表示 CPU 模式

### 访问类型

- P：存在位（Present bit），1 表示在内存上
- DPL：描述符特权等级（Descriptor privilege level），0～3
- S：描述符类型位（Descriptor type bit），0 表示系统段，1 表示数据段或代码段
- Type：
| X | D/C | R/W | A |
    - X：0，数据段
        - D：Direction bit，0 表示向上增长，1 表示向下增长
        - W：Writable bit，0 表示不可写，1 表示可写
        - A：Accessed bit，1 表示被 CPU 访问过
    - X：1，代码段
        - C：Conforming bit，0 表示只能在 DPL 对应的特权级执行，1 表示可以在
          DPL 对应及以下特权级执行
        - R：Readable bit，0 表示不可读，1 表示可读
        - A：Accessed bit，1 表示被 CPU 访问过

### 标志

- G：Granularity flag，颗粒度，1 表示 4KiB，0 表示 1 B
- DB：Size flag：1 表示 32 位保护模式段，0 表示 16 位
- L：Long-mode flag：1 表示 64 位段
- A：置 0

## 段选择器

段选择器（Selector）类似实模式中的段地址，它指明了段描述符在 GDT 中的偏移位置，还带有校验功能。

结构：
- 0～1：RPL
- 2：0 表示全局描述符，1 表示本地描述符
- 3～15：GDT 索引

## 保护模式

```asm
mov eax, cr0
or al, 1
mov cr0, eax

; 开启保护模式后立即使用一个远跳转
jmp <segment:offset>
```

## 参考资料

[GDT - OSDev](https://wiki.osdev.org/GDT)
[Segment Selector - OSDev](https://wiki.osdev.org/Segment_Selector)
[Global Descriptor Table - WIKI](https://en.wikipedia.org/wiki/Global_Descriptor_Table)
[Организация памяти](https://habr.com/en/articles/128991/)
