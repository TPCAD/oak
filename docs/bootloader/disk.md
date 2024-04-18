# Disk

这里的硬盘特指机械硬盘。

## 物理结构

<img src="https://upload.wikimedia.org/wikipedia/commons/0/02/Cylinder_Head_Sector.svg" style="zoom: 60%">

机械硬盘最主要的结构是盘片（Platter）和磁头（Head）。盘片由磁性材料制成，磁头在
通过改变盘片区域的磁性来存储数据。盘片旋转时，若磁头保持在一个位置上，则每个磁
头会在磁盘表面划出一个圆形轨迹，称为磁道（Track）。不同盘片的相同磁道会构成一个
圆柱面，称为柱面（Cylinder）。将圆形盘片分成若干相同大小的扇形区域，称为扇区
（Sector）。通过 Cylinder-Head-Sector 就可以定位一块数据的位置。因此，硬盘读写
的最小单位是一个扇区。

## CHS 与 LBA

BIOS 有两种方法定位硬盘扇区，一个是 CHS，一个是 LBA。

CHS 模式即 Cylinder-Head-Sector，通过柱面、磁头和扇区来定位。柱面和磁头从 0 开
始计数，而扇区从 1 开始计数。

LBA（Logic Block Addressing）将硬盘抽象成一串固定大小的逻辑块，逻辑块从 0 开始
计数，可以直接通过逻辑块号定位扇区。

LBA 和 CHS 可以相互转换。

## 读取硬盘

### BIOS 中断，CHS 模式

BIOS 中断 `0x13h` 提供硬盘相关服务。读硬盘的功能号为 `0x2` 。

参数表：

| Parameters   | Function    |
|--------------- | --------------- |
| AH   | 0x02   |
| AL   | sectors to read count   |
| CH   | cylinder   |
| CL   | sector  |
| DH   | head  |
| DL   | drive  |
| ES:BX   | buffer address pointer |

返回值：

| Result   | Function    |
|--------------- | --------------- |
| CF   | set on error, clear if no error   |
| AH   | return code   |
| AL   | actual sectors read count   |

示例：

```asm
mov ah, 0x2 ; function number
mov al, 1 ; sectors count to read
mov ch, 0 ; cylinder
mov cl, 1 ; sector
mov dh, 0 ; head
mov dl, 0x80 ; drive

; clear es
xor bx, bx
mov es, bx
mov bx, 0x1000 ; buffer pointer
int 0x13 ; interruption
```

### BIOS 中断，LBA 模式

想要使用 LBA 模式，BIOS 需要 Extended Mode。

参数表：

| Registers   | Description    |
|--------------- | --------------- |
| AH   | 0x42   |
| DL   | drive   |
| DS:SI   | pointer to DAP structure   |

DAP（Disk Address Packet）：

| Offset(bytes)   | Size(bytes) | Description    |
|--------------- |---------| --------------- |
| 0   |1| Size of DAP(set to 0x10, 16 bytes)   |
| 1   |1| Always 0   |
| 2   |2| Sectors count to read   |
| 4   |4| buffer pointer(16 bits segment:16 bits offset)|
| 4   |4| Lower 32 bits of 48 bits LBA address|
| 4   |4| Upper 16 bits of 48 bits LBA address|

Notes:

1. offset must be placed before segment since x86 is little-endian
2. if the disk doesn't support LBA, BIOS will convert the LBA to CHS
automatically. So the function still work
3. teh buffer must be 16-bit aligned

返回值：

| Registers | Description |
| --------- | ----------- |
| CF | Set On Error, Clear If No Error |
| AH | Return Code |

示例：

```asm
mov si, DAPACK
mov ah, 0x42
mov dl, 0x80
int 0x13

DAPACK:
	db 0x10 ; size of packet (16 bytes)
	db 0 ; always 0
	dw 1 ; number of sectors to read
	dw 0x1000 ; buffer offset address
	dw 0 ; buffer segment address
	dd 0 ; lower 32 bits of LBA address
	dd 0 ; upper 16 bits of LBA address
```

#### 检查是否支持 Extended Mode

参数表：

| Registers | Description |
| --------- | ----------- |
| AH | 0x41 |
| DL | drive |
| BX | 0x55AA |

若不支持则 CF 位设为 1。

### 非 BIOS 中断，LBA 模式

```asm
mov edi, 0x1000
mov ecx, 0
mov bl, 1

call read_disk

read_disk:
	mov dx, 0x1f2
	mov al, bl
	out dx, al

	inc dx,
	mov al, cl
	out dx, al

	; 0x1f4
	inc dx
	shr ecx, 8
	mov al, cl
	out dx, al
	
	; 0x1f5
	inc dx
	shr ecx, 8
	mov al, cl
	out dx, al

	inc dx
	shr ecx, 8
	and cl, 0b1111

	mov al, 0b1110_0000
	or al, cl
	out dx, al

	; 0x1f7
	inc dx
	mov al, 0x20
	out dx, al

	xor ecx, ecx
	mov cl, bl

.read_sector:
	push cx
	call .wait
	call .read
	pop cx
	loop .read_sector

	ret

.wait:
	mov dx, 0x1f7
	.check:
		in al, dx
		jmp $+2
		jmp $+2
		jmp $+2
		and al, 0b1000_1000
		cmp al, 0b0000_1000
		jnz .check
	ret

.read:
	mov dx, 0x1f0
	mov cx, 256
	.read_word:
		in ax, dx
		jmp $+2
		jmp $+2
		jmp $+2
		mov [edi], ax
		add edi, 2
		loop .read_word
	ret
```

## 参考资料

[PIO Mode - OSDev](https://wiki.osdev.org/ATA_PIO_Mode)
[I/O Ports - OSDev](https://wiki.osdev.org/I/O_Ports)
[ATA Read Write Sectors - OSDev](https://wiki.osdev.org/ATA_read/write_sectors)
[INT 13H - WIKI](https://en.wikipedia.org/wiki/INT_13H)
[Disk Access Using BIOS - OSDev](https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h))
