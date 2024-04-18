compile assembly file

```bash
nasm -f bin boot.asm -o boot.bin
```

create disk image

```bash
bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat master.img
```

write boot.bin into master.img

```bash
dd if=boot.bin of=master.img bs=512 count=1 conv=notrunc
```

generate bochs config file and modify it

```language
ata0-master: type=disk, path="master.img", mode="flat"

boot: disk

display_library: x, options="gui_debug"
```
