SRC:=.
BUILD:=../build
BUILD_BOOT:=$(BUILD)/boot
BUILD_KERNEL:=$(BUILD)/kernel
BUILD_LIB:=$(BUILD)/lib

ENTRYPOINT:=0x10000

CFLAGS:= -m32 # 32 bits program
CFLAGS+= -fno-builtin # no gcc builtin function
CFLAGS+= -nostdinc # no std include file
CFLAGS+= -fno-pic # no position independent code
CFLAGS+= -fno-pie # no position independent executable file
CFLAGS+= -nostdlib # no stdlib
CFLAGS+= -fno-stack-protector
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g

INCLUDE:=-I$(SRC)/include

$(BUILD_BOOT):
	@mkdir -p $(BUILD)/boot
$(BUILD_KERNEL):
	@mkdir -p $(BUILD)/kernel
$(BUILD_LIB):
	@mkdir -p $(BUILD)/lib

# bootloader
$(BUILD_BOOT)/%.bin: $(SRC)/boot/%.asm | $(BUILD_BOOT)
	nasm -f bin $< -o $@

# kernel
# src/kernel/*.asm
$(BUILD_KERNEL)/%.o: $(SRC)/kernel/%.asm | $(BUILD_KERNEL)
	nasm -gdwarf -f elf32 $< -o $@

# src/kernel/*.c
$(BUILD_KERNEL)/%.o: $(SRC)/kernel/%.c | $(BUILD_KERNEL)
	i686-elf-gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

# src/lib/*.c
$(BUILD_LIB)/%.o: $(SRC)/lib/%.c | $(BUILD_LIB)
	i686-elf-gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD_KERNEL)/kernel.bin: $(BUILD_KERNEL)/start.o \
	$(BUILD_KERNEL)/main.o \
	$(BUILD_KERNEL)/io.o \
	$(BUILD_LIB)/string.o \
	$(BUILD_KERNEL)/console.o \
	$(BUILD_LIB)/vsprintf.o \
	$(BUILD_KERNEL)/printk.o \
	$(BUILD_KERNEL)/assert.o \
	$(BUILD_KERNEL)/debug.o \
	$(BUILD_KERNEL)/global.o \
	$(BUILD_KERNEL)/interrupt.o \
	$(BUILD_KERNEL)/handler.o \
	$(BUILD_LIB)/stdlib.o \
	$(BUILD_KERNEL)/clock.o \
	$(BUILD_KERNEL)/time.o | $(BUILD_KERNEL)
	i686-elf-ld -m elf_i386 -static $^ -o $@ -Ttext $(ENTRYPOINT)

$(BUILD_KERNEL)/system.bin: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	objcopy -O binary $< $@

$(BUILD_KERNEL)/system.map: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	nm $< | sort > $@

# image
$(BUILD)/master.img: $(BUILD_BOOT)/boot.bin \
	$(BUILD_BOOT)/loader.bin \
	$(BUILD_KERNEL)/system.bin \
	$(BUILD_KERNEL)/system.map
	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD_BOOT)/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD_BOOT)/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD_KERNEL)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

.PHONY: image
image: $(BUILD)/master.img

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: bochs
bochs: $(BUILD)/master.img
	bochs -q -f ../debug/bochsrc

.PHONY: bochs-dbg
bochs-dbg: $(BUILD)/master.img
	bochs-gdb -q -f ../debug/bochsrc.gdb

.PHONY: qemu
qemu: $(BUILD)/master.img
	qemu-system-i386 -m 32M -boot c -hda $< -audiodev pa,id=hda -machine pcspk-audiodev=hda

.PHONY: qemu-dbg
qemu-dbg: $(BUILD)/master.img
	qemu-system-i386 -s -S -m 32M -boot c -hda $< -audiodev pa,id=hda -machine pcspk-audiodev=hda
