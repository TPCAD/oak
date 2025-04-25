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
