# ARCH
ARCH:=x86

# source directories
SRC:=.
BOOT_DIR:=$(SRC)/arch/$(ARCH)/boot
KERNEL_DIR:=$(SRC)/kernel

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
# CFLAGS+= -DOAK
# CFLAGS:=$(strip ${CFLAGS})

$(BUILD_BOOT):
	@mkdir -p $@
$(BUILD_KERNEL):
	@mkdir -p $@

# bootloader
$(BUILD_BOOT)/%.o: $(BOOT_DIR)/%.s | $(BUILD_BOOT)
	$(AS) -o $@ $<

$(BUILD_BOOT)/boot.bin: $(BUILD_BOOT)/boot.o
	ld -o $@ --oformat binary -Ttext=0x7c00 $<
$(BUILD_BOOT)/loader.bin: $(BUILD_BOOT)/loader.o
	ld -o $@ --oformat binary -Ttext=0x1000 $<

# kernel
$(BUILD_KERNEL)/%.o: $(KERNEL_DIR)/%.S | $(BUILD_KERNEL)
	as --32 -g $< -o $@
$(BUILD_KERNEL)/kernel.bin: $(BUILD_KERNEL)/start.o | $(BUILD_KERNEL)
	ld $(LDFLAGS) $^ -o $@
$(BUILD_KERNEL)/system.bin: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	objcopy -O binary --remove-section=.note.gnu.property $< $@
$(BUILD_KERNEL)/system.map: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	nm $< | sort > $@

$(BUILD)/master.img: $(BUILD_BOOT)/boot.bin \
	$(BUILD_BOOT)/loader.bin \
	$(BUILD_KERNEL)/system.bin
	# $(BUILD_KERNEL)/system.map
	qemu-img create $@ 16M
	dd bs=512 count=1 conv=notrunc if=$(BUILD_BOOT)/boot.bin of=$@
	dd bs=512 count=4 seek=2 conv=notrunc if=$(BUILD_BOOT)/loader.bin of=$@
	dd bs=512 count=200 seek=10 conv=notrunc if=$(BUILD_KERNEL)/system.bin of=$@

IMAGE:=$(BUILD)/master.img

.PHONY: image
image: $(IMAGE)

# bochs
.PHONY: bochs
bochs: $(IMAGE)
	bochs -q -f ../debug/bochsrc -unlock

.PHONY: clean
clean:
	rm -rf $(BUILD)
