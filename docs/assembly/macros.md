# Macro in NASM

```asm
%macro <macro_name> <para_num>
; instructions
%macroend
```

使用 `%<num>` 表示参数。

```asm
%macro INTERRUPT_HANDLER 2
interrupt_handler_%1:
%ifn %2
	push 0x20240419
%endif
	push %1
	jmp interrupt_entry
%endmacro
```
