# ==================== bootloader ====================
$(BUILD_BOOT)/%.o: $(BOOT_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) -o $@ $<

$(BUILD_BOOT)/boot.bin: $(BUILD_BOOT)/boot.o
	ld -o $@ --oformat binary -Ttext=0x7c00 $<
$(BUILD_BOOT)/loader.bin: $(BUILD_BOOT)/loader.o
	ld -o $@ --oformat binary -Ttext=0x1000 $<

# ==================== compile ====================
KERNEL_SRC:=$(shell find $(KERNEL_DIR) $(ARCH_DIR) $(LIB_DIR) -name "*.[cS]")
OBJ:=$(patsubst ./%, $(BUILD)/%.o, $(KERNEL_SRC))

$(BUILD)/%.c.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -g -c $< -o $@

$(BUILD)/%.S.o: %.S
	@mkdir -p $(@D)
	$(AS) --32 -g $< -o $@

$(BUILD_BUILTIN)/%.o.out: $(BUILD_BUILTIN)/%.o
	ld -m elf_i386 -static $^ -o $@ -Ttext 0x1001000

# ==================== kernel ====================
$(BUILD_KERNEL)/kernel.bin: $(OBJ) $(BUILD_BUILTIN)/ash.c.o
	ld $(LDFLAGS) $^ -o $@
$(BUILD_KERNEL)/system.bin: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	objcopy -O binary --remove-section=.note.gnu.property $< $@
$(BUILD_KERNEL)/system.map: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	nm $< | sort > $@

