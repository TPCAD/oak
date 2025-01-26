SRC:=.
BUILD:=../build
BUILD_BOOT:=$(BUILD)/boot

$(BUILD_BOOT):
	@mkdir -p $@

X86_64_BOOT_DIR:=$(SRC)/arch/boot

$(BUILD_BOOT)/%.o: $(X86_64_BOOT_DIR)/%.s | $(BUILD_BOOT)
	$(AS) -o $@ $<

$(BUILD_BOOT)/%.bin: $(BUILD_BOOT)/%.o
	ld -o $@ --oformat binary -Ttext=0x7c00 $<

$(BUILD)/master.img: $(BUILD_BOOT)/boot.bin
	qemu-img create $@ 16M
	qemu-img dd bs=512 count=1 if=$< of=$@

IMAGE:=$(BUILD)/master.img

.PHONY: image
image: $(IMAGE)

.PHONY: clean
clean:
	rm -rf $(BUILD)
