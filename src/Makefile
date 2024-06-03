SRC:=.
BUILD:=../build
BUILD_BOOT:=$(BUILD)/boot
BUILD_KERNEL:=$(BUILD)/kernel
BUILD_LIB:=$(BUILD)/lib

MULTIBOOT2:=0x10000
ENTRYPOINT:=0x10040

CFLAGS:= -m32 # 32 bits program
CFLAGS+= -fno-builtin # no gcc builtin function
CFLAGS+= -nostdinc # no std include file
CFLAGS+= -fno-pic # no position independent code
CFLAGS+= -fno-pie # no position independent executable file
CFLAGS+= -nostdlib # no stdlib
CFLAGS+= -fno-stack-protector
CFLAGS:=$(strip ${CFLAGS})

LDFLAGS:= -m elf_i386 \
	  -static \
	  -Ttext $(ENTRYPOINT) \
	  --section-start=.multiboot2=$(MULTIBOOT2)

LDFLAGS:=$(strip $(LDFLAGS))

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
	$(BUILD_KERNEL)/time.o \
	$(BUILD_KERNEL)/rtc.o \
	$(BUILD_KERNEL)/memory.o \
	$(BUILD_LIB)/bitmap.o \
	$(BUILD_KERNEL)/task.o \
	$(BUILD_KERNEL)/schedule.o \
	$(BUILD_KERNEL)/gate.o \
	$(BUILD_LIB)/syscall.o \
	$(BUILD_LIB)/list.o \
	$(BUILD_KERNEL)/thread.o \
	$(BUILD_KERNEL)/mutex.o \
	$(BUILD_KERNEL)/keyboard.o \
	$(BUILD_LIB)/fifo.o \
	$(BUILD_LIB)/printf.o \
	$(BUILD_KERNEL)/arena.o \
	$(BUILD_KERNEL)/ide.o | $(BUILD_KERNEL)
	i686-elf-ld $(LDFLAGS) $^ -o $@

$(BUILD_KERNEL)/system.bin: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	objcopy -O binary $< $@

$(BUILD_KERNEL)/system.map: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	nm $< | sort > $@

# image
$(BUILD)/master.img: $(BUILD_BOOT)/boot.bin \
	$(BUILD_BOOT)/loader.bin \
	$(BUILD_KERNEL)/system.bin \
	$(BUILD_KERNEL)/system.map
	# create a 16M disk image
	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	# write boot.bin to image
	dd if=$(BUILD_BOOT)/boot.bin of=$@ bs=512 count=1 conv=notrunc
	# write loader.bin to image
	dd if=$(BUILD_BOOT)/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
	# test if system.bin is less than 100k
	test -n "$$(find $(BUILD_KERNEL)/system.bin -size -100k)"
	# write system.bin to image
	dd if=$(BUILD_KERNEL)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

# iso
$(BUILD)/kernel.iso: $(BUILD_KERNEL)/kernel.bin $(SRC)/utils/grub.cfg
	grub-file --is-x86-multiboot2 $<
	mkdir -p $(BUILD)/iso/boot/grub
	cp $< $(BUILD)/iso/boot
	cp $(SRC)/utils/grub.cfg $(BUILD)/iso/boot/grub
	grub-mkrescue -o $@ $(BUILD)/iso

.PHONY: image
image: $(BUILD)/master.img

.PHONY: clean
clean:
	rm -rf $(BUILD)

# bochs

.PHONY: bochs
bochs: $(BUILD)/master.img
	bochs -q -f ../debug/bochsrc -unlock

.PHONY: bochs-grub
bochs-grub: $(BUILD)/kernel.iso
	bochs -q -f ../debug/bochsrc.grub -unlock

.PHONY: bochs-dbg
bochs-dbg: $(BUILD)/master.img
	bochs-gdb -q -f ../debug/bochsrc.gdb -unlock

# qemu

QEMU:= qemu-system-i386 \
       -m 32M \
       -audiodev pa,id=hda \
       -machine pcspk-audiodev=hda \
       -rtc base=localtime \

QEMU_DISK:=-boot c \
	   -drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw\

QEMU_CDROM:=-boot d \
	   -drive file=$(BUILD)/kernel.iso,media=cdrom \

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu: $(BUILD)/master.img
	$(QEMU) $(QEMU_DISK)
	
.PHONY: qemu-grub
qemu-grub: $(BUILD)/kernel.iso
	$(QEMU) $(QEMU_CDROM)

.PHONY: qemu-dbg
qemu-dbg: $(BUILD)/master.img
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)
