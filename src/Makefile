SRC:=.
BUILD:=../build
BUILD_BOOT:=$(BUILD)/boot
BUILD_KERNEL:=$(BUILD)/kernel
BUILD_FS:=$(BUILD)/fs
BUILD_LIB:=$(BUILD)/lib
BUILD_BUILTIN:=$(BUILD)/builtin

MULTIBOOT2:=0x10000
ENTRYPOINT:=0x10040

CFLAGS:= -m32 # 32 bits program
CFLAGS+= -fno-builtin # no gcc builtin function
CFLAGS+= -nostdinc # no std include file
CFLAGS+= -fno-pic # no position independent code
CFLAGS+= -fno-pie # no position independent executable file
CFLAGS+= -nostdlib # no stdlib
CFLAGS+= -fno-stack-protector
CFLAGS+= -DOAK
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
$(BUILD_FS):
	@mkdir -p $(BUILD)/fs
$(BUILD_BUILTIN):
	@mkdir -p $(BUILD)/builtin

# bootloader
$(BUILD_BOOT)/%.bin: $(SRC)/boot/%.asm | $(BUILD_BOOT)
	nasm -f bin $< -o $@

# kernel
# src/kernel/*.asm
$(BUILD_KERNEL)/%.o: $(SRC)/kernel/%.asm | $(BUILD_KERNEL)
	nasm -gdwarf -f elf32 $< -o $@

# src/kernel/*.c
$(BUILD_KERNEL)/%.o: $(SRC)/kernel/%.c | $(BUILD_KERNEL)
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

# src/lib/*.c
$(BUILD_LIB)/%.o: $(SRC)/lib/%.c | $(BUILD_LIB)
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@
# src/lib/*.asm
$(BUILD_LIB)/%.o: $(SRC)/lib/%.asm | $(BUILD_LIB)
	nasm -gdwarf -f elf32 $< -o $@

# src/fs/*.c
$(BUILD_FS)/%.o: $(SRC)/fs/%.c | $(BUILD_FS)
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

# src/builtin/*.c
$(BUILD_BUILTIN)/%.o: $(SRC)/builtin/%.c | $(BUILD_BUILTIN)
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@
# src/builtin/*.asm
$(BUILD_BUILTIN)/%.o: $(SRC)/builtin/%.asm | $(BUILD_BUILTIN)
	nasm -gdwarf -f elf32 $< -o $@

# build libc
$(BUILD_LIB)/libc.o: $(BUILD_LIB)/assert.o \
	$(BUILD_LIB)/crt.o \
	$(BUILD_LIB)/crt1.o \
	$(BUILD_LIB)/printf.o \
	$(BUILD_LIB)/stdlib.o \
	$(BUILD_LIB)/string.o \
	$(BUILD_LIB)/syscall.o \
	$(BUILD_LIB)/time.o \
	$(BUILD_LIB)/vsprintf.o | $(BUILD_BUILTIN)
	ld -m elf_i386 -r $^ -o $@

BUILDIN_APPS := \
		$(BUILD_BUILTIN)/ash.out \
		$(BUILD_BUILTIN)/cat.out \
		$(BUILD_BUILTIN)/echo.out \
		$(BUILD_BUILTIN)/env.out \
		$(BUILD_BUILTIN)/err.out \
		$(BUILD_BUILTIN)/init.out \
		$(BUILD_BUILTIN)/ls.out \
		$(BUILD_BUILTIN)/dup.out \

$(BUILD_BUILTIN)/%.out: $(BUILD_BUILTIN)/%.o \
	$(BUILD_LIB)/libc.o | $(BUILD_BUILTIN)
	ld -m elf_i386 -static $^ -o $@ -Ttext 0x1001000

$(BUILD_KERNEL)/kernel.bin: $(BUILD_KERNEL)/start.o \
	$(BUILD_LIB)/bitmap.o \
	$(BUILD_LIB)/fifo.o \
	$(BUILD_LIB)/list.o \
	$(BUILD_LIB)/stdlib.o \
	$(BUILD_LIB)/string.o \
	$(BUILD_LIB)/syscall.o \
	$(BUILD_LIB)/vsprintf.o \
	$(BUILD_KERNEL)/arena.o \
	$(BUILD_KERNEL)/assert.o \
	$(BUILD_KERNEL)/buffer.o \
	$(BUILD_KERNEL)/clock.o \
	$(BUILD_KERNEL)/console.o \
	$(BUILD_KERNEL)/debug.o \
	$(BUILD_KERNEL)/device.o \
	$(BUILD_KERNEL)/execve.o \
	$(BUILD_KERNEL)/gate.o \
	$(BUILD_KERNEL)/global.o \
	$(BUILD_KERNEL)/handler.o \
	$(BUILD_KERNEL)/ide.o \
	$(BUILD_KERNEL)/interrupt.o \
	$(BUILD_KERNEL)/io.o \
	$(BUILD_KERNEL)/keyboard.o \
	$(BUILD_KERNEL)/main.o \
	$(BUILD_KERNEL)/memory.o \
	$(BUILD_KERNEL)/mutex.o \
	$(BUILD_KERNEL)/printk.o \
	$(BUILD_KERNEL)/ramdisk.o \
	$(BUILD_KERNEL)/rtc.o \
	$(BUILD_KERNEL)/schedule.o \
	$(BUILD_KERNEL)/serial.o \
	$(BUILD_KERNEL)/system.o \
	$(BUILD_KERNEL)/task.o \
	$(BUILD_KERNEL)/thread.o \
	$(BUILD_KERNEL)/time.o \
	$(BUILD_FS)/bmap.o \
	$(BUILD_FS)/dev.o \
	$(BUILD_FS)/file.o \
	$(BUILD_FS)/inode.o \
	$(BUILD_FS)/namei.o \
	$(BUILD_FS)/pipe.o \
	$(BUILD_FS)/stat.o \
	$(BUILD_FS)/super.o \
	| $(BUILD_KERNEL)
	ld $(LDFLAGS) $^ -o $@

$(BUILD_KERNEL)/system.bin: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	objcopy -O binary $< $@

$(BUILD_KERNEL)/system.map: $(BUILD_KERNEL)/kernel.bin | $(BUILD_KERNEL)
	nm $< | sort > $@

# master.img
$(BUILD)/master.img: $(BUILD_BOOT)/boot.bin \
	$(BUILD_BOOT)/loader.bin \
	$(BUILD_KERNEL)/system.bin \
	$(BUILD_KERNEL)/system.map \
	$(SRC)/utils/master.sfdisk \
	$(BUILDIN_APPS)
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
	# disk partition
	sfdisk $@ < $(SRC)/utils/master.sfdisk
	# mount device
	sudo losetup /dev/loop0 --partscan $@
	# create fs
	sudo mkfs.minix -1 -n 14 /dev/loop0p1
	# mount fs
	sudo mount /dev/loop0p1 /mnt
	# change owner
	sudo chown ${USER} /mnt
	# create dir
	mkdir -p /mnt/bin
	mkdir -p /mnt/dev
	mkdir -p /mnt/mnt
	# create file
	echo "hello oak from root directory!" > /mnt/hello.txt
	# copy file
	for app in $(BUILDIN_APPS); \
	do \
		cp $$app /mnt/bin; \
	done
	# unmount fs
	sudo umount /mnt
	# unmount device
	sudo losetup -d /dev/loop0

# slave.img
$(BUILD)/slave.img: $(SRC)/utils/slave.sfdisk
	yes | bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@
	# disk partition
	sfdisk $@ < $(SRC)/utils/slave.sfdisk
	# mount device
	sudo losetup /dev/loop0 --partscan $@
	# create fs
	sudo mkfs.minix -1 -n 14 /dev/loop0p1
	# mount fs
	sudo mount /dev/loop0p1 /mnt
	# change owner
	sudo chown ${USER} /mnt
	
	mkdir -p /mnt/home
	echo "slave home directory file!" > /mnt/home/hello.txt
	# create file
	echo "slave root directory file!" > /mnt/hello.txt
	# unmount fs
	sudo umount /mnt
	# unmount device
	sudo losetup -d /dev/loop0

# kernel.iso
$(BUILD)/kernel.iso: $(BUILD_KERNEL)/kernel.bin $(SRC)/utils/grub.cfg
	grub-file --is-x86-multiboot2 $<
	mkdir -p $(BUILD)/iso/boot/grub
	cp $< $(BUILD)/iso/boot
	cp $(SRC)/utils/grub.cfg $(BUILD)/iso/boot/grub
	grub-mkrescue -o $@ $(BUILD)/iso

IMAGES:=$(BUILD)/master.img $(BUILD)/slave.img


# ========== emulator ==========

# bochs

.PHONY: bochs
bochs: $(IMAGES)
	bochs -q -f ../debug/bochsrc -unlock

.PHONY: bochs-grub
bochs-grub: $(BUILD)/kernel.iso
	bochs -q -f ../debug/bochsrc.grub -unlock

.PHONY: bochs-dbg
bochs-dbg: $(IMAGES)
	bochs-gdb -q -f ../debug/bochsrc.gdb -unlock

# qemu

QEMU:= qemu-system-i386 # virtual machine
QEMU+= -m 32M # memory
QEMU+= -audiodev pa,id=hda # audio device
QEMU+= -machine pcspk-audiodev=hda # pc speaker
QEMU+= -rtc base=localtime # local time
# master disk
QEMU+= -drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw
# slave disk
QEMU+= -drive file=$(BUILD)/slave.img,if=ide,index=1,media=disk,format=raw
QEMU+= -chardev stdio,mux=on,id=com1 # character device 1, terminal
# QEMU+= -chardev vc,mux=on,id=com1 # character device 1, vc in qemu
QEMU+= -chardev vc,mux=on,id=com2 # character device 2, vc in qemu
# QEMU+= -chardev udp,mux=on,id=com2,port=7777,ipv4=on # character device 2,udp
QEMU+= -serial chardev:com1 # serial 1
QEMU+= -serial chardev:com2 # serial 2

QEMU_DISK:=-boot c

QEMU_CDROM:=-boot d \
	    -drive file=$(BUILD)/kernel.iso,media=cdrom

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu: $(IMAGES)
	$(QEMU) $(QEMU_DISK)
	
.PHONY: qemu-grub
qemu-grub: $(BUILD)/kernel.iso
	$(QEMU) $(QEMU_CDROM)

.PHONY: qemu-dbg
qemu-dbg: $(IMAGES)
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

# ========== mount ==========
.PHONY: mount-master
mount-master: $(BUILD)/master.img
	sudo losetup /dev/loop0 --partscan $<
	sudo mount /dev/loop0p1 /mnt
	sudo chown ${USER} /mnt

.PHONY: umount-master
umount-master: /dev/loop0
	-sudo umount /mnt
	-sudo losetup -d $<

.PHONY: mount-slave
mount-slave: $(BUILD)/slave.img
	sudo losetup /dev/loop1 --partscan $<
	sudo mount /dev/loop1p1 /mnt
	sudo chown ${USER} /mnt

.PHONY: umount-slave
umount-slave: /dev/loop1
	-sudo umount /mnt
	-sudo losetup -d $<

# ========== misc ==========

# build .img only
.PHONY: image
image: $(IMAGES)

# remove build directory
.PHONY: clean
clean:
	rm -rf $(BUILD)
