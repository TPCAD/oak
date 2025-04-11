# architecture
ARCH:=x86

# source directories
SRC:=.
ARCH_DIR:=$(SRC)/arch/$(ARCH)
KERNEL_DIR:=$(SRC)/kernel
BOOT_DIR:=$(ARCH_DIR)/boot
LIB_DIR:=$(SRC)/lib
UTILS_DIR:=$(SRC)/utils

# build directories
BUILD:=../build
BUILD_BOOT:=$(BUILD)/boot
BUILD_KERNEL:=$(BUILD)/kernel

# compile/link arguments
LDFLAGS:= -m elf_i386 \
		  -static \
		  -T linker.ld

CFLAGS:= -m32 # 32 bits program
CFLAGS+= -fno-builtin # no gcc builtin function
CFLAGS+= -nostdinc # no std include file
CFLAGS+= -fno-pic # no position independent code
CFLAGS+= -fno-pie # no position independent executable file
CFLAGS+= -nostdlib # no stdlib
CFLAGS+= -fno-stack-protector
CFLAGS+= -I$(SRC)/include
CFLAGS+= -I$(SRC)/arch/$(ARCH)/include
CFLAGS+= -O0
# CFLAGS+= -DOAK
# CFLAGS:=$(strip ${CFLAGS})

# bootloader
$(BUILD_BOOT)/%.o: $(BOOT_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) -o $@ $<

$(BUILD_BOOT)/boot.bin: $(BUILD_BOOT)/boot.o
	ld -o $@ --oformat binary -Ttext=0x7c00 $<
$(BUILD_BOOT)/loader.bin: $(BUILD_BOOT)/loader.o
	ld -o $@ --oformat binary -Ttext=0x1000 $<

KERNEL_SRC:=$(shell find $(KERNEL_DIR) $(ARCH_DIR) $(LIB_DIR) -name "*.[cS]")
OBJ:=$(patsubst ./%, $(BUILD)/%.o, $(KERNEL_SRC))

$(BUILD)/%.c.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -g -c $< -o $@

$(BUILD)/%.S.o: %.S
	@mkdir -p $(@D)
	$(AS) --32 -g $< -o $@

# kernel
$(BUILD_KERNEL)/kernel.bin: $(OBJ)
	ld $(LDFLAGS) $^ -o $@
$(BUILD_KERNEL)/system.bin: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	objcopy -O binary --remove-section=.note.gnu.property $< $@
$(BUILD_KERNEL)/system.map: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	nm $< | sort > $@

# kernel.iso(boot with grub)
$(BUILD)/kernel.iso: $(BUILD_KERNEL)/kernel.bin $(UTILS_DIR)/grub.cfg
	grub-file --is-x86-multiboot2 $<
	mkdir -p $(BUILD)/iso/boot/grub
	cp $< $(BUILD)/iso/boot
	cp $(UTILS_DIR)/grub.cfg $(BUILD)/iso/boot/grub
	grub-mkrescue -o $@ $(BUILD)/iso

$(BUILD)/master.img: $(BUILD_BOOT)/boot.bin \
	$(BUILD_BOOT)/loader.bin \
	$(BUILD_KERNEL)/system.bin \
	$(BUILD_KERNEL)/system.map
	qemu-img create $@ 16M
	dd bs=512 count=1 conv=notrunc if=$(BUILD_BOOT)/boot.bin of=$@
	dd bs=512 count=4 seek=2 conv=notrunc if=$(BUILD_BOOT)/loader.bin of=$@
	dd bs=512 count=200 seek=10 conv=notrunc if=$(BUILD_KERNEL)/system.bin of=$@

$(BUILD)/slave.img:
	qemu-img create $@ 32M

IMAGE:=$(BUILD)/master.img $(BUILD)/slave.img

.PHONY: image
image: $(IMAGE)

# bochs
.PHONY: bochs
bochs: $(IMAGE)
	bochs -q -f ../debug/bochsrc -unlock

# bochs-grub
.PHONY: bochs-grub
bochs-grub: $(BUILD)/kernel.iso
	bochs -q -f ../debug/bochsrc_grub -unlock

# bochs-gdb
.PHONY: bochs-gdb
bochs-gdb: $(IMAGE)
	bochs-gdb -q -f ../debug/bochsrc_gdb -unlock

QEMU:= qemu-system-i386
QEMU+= -m 32M
QEMU+= -audiodev pa,id=hda
QEMU+= -machine pcspk-audiodev=hda
QEMU+= -drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw
QEMU+= -drive file=$(BUILD)/slave.img,if=ide,index=1,media=disk,format=raw

QEMU_DISK:= -boot c

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu: $(IMAGE)
	$(QEMU) $(QEMU_DISK)

.PHONY: qemu-gdb
qemu-gdb: $(IMAGE)
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

.PHONY: clean
clean:
	rm -rf $(BUILD)
