SRC:=.
BUILD:=../build
BUILD_BOOT:=$(BUILD)/boot

$(BUILD_BOOT):
	@mkdir -p $@

BOOT_DIR:=$(SRC)/arch/boot

$(BUILD_BOOT)/%.o: $(BOOT_DIR)/%.s | $(BUILD_BOOT)
	$(AS) -o $@ $<

$(BUILD_BOOT)/boot.bin: $(BUILD_BOOT)/boot.o
	ld -o $@ --oformat binary -Ttext=0x7c00 $<
$(BUILD_BOOT)/loader.bin: $(BUILD_BOOT)/loader.o
	ld -o $@ --oformat binary -Ttext=0x1000 $<

$(BUILD)/master.img: $(BUILD_BOOT)/boot.bin $(BUILD_BOOT)/loader.bin
	qemu-img create $@ 16M
	dd bs=512 count=1 conv=notrunc if=$(BUILD_BOOT)/boot.bin of=$@
	dd bs=512 count=4 seek=2 conv=notrunc if=$(BUILD_BOOT)/loader.bin of=$@

IMAGE:=$(BUILD)/master.img

.PHONY: image
image: $(IMAGE)

.PHONY: clean
clean:
	rm -rf $(BUILD)
