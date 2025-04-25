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
	$(BUILD_KERNEL)/system.map \
	$(UTILS_DIR)/master.sfdisk
	qemu-img create $@ 16M
	dd bs=512 count=1 conv=notrunc if=$(BUILD_BOOT)/boot.bin of=$@
	dd bs=512 count=4 seek=2 conv=notrunc if=$(BUILD_BOOT)/loader.bin of=$@
	dd bs=512 count=200 seek=10 conv=notrunc if=$(BUILD_KERNEL)/system.bin of=$@
	# 硬盘分区
	sfdisk $@ < $(UTILS_DIR)/master.sfdisk
	# 挂载磁盘镜像文件，自动识别分区
	sudo losetup /dev/loop0 --partscan $@
	# 在硬盘分区创建 minix 文件系统
	sudo mkfs.minix -1 -n 14 /dev/loop0p1
	# 挂载分区
	sudo mount /dev/loop0p1 /mnt
	# 切换用户
	sudo chown $(USER) /mnt
	# 创建文件
	mkdir -p /mnt/bin
	mkdir -p /mnt/dev
	mkdir -p /mnt/mnt
	mkdir -p /mnt/home
	mkdir -p /mnt/d1/d2/d3
	echo "hello oak from root directory!" > /mnt/hello.txt
	echo "hello oak from home directory!" > /mnt/home/hello.txt
	# 卸载镜像文件
	sudo umount /mnt
	sudo losetup -d /dev/loop0

$(BUILD)/slave.img:
	qemu-img create $@ 32M
	sfdisk $@ < $(UTILS_DIR)/master.sfdisk
	sudo losetup /dev/loop0 --partscan $@
	sudo mkfs.minix -1 -n 14 /dev/loop0p1
	sudo mount /dev/loop0p1 /mnt
	sudo chown $(USER) /mnt
	mkdir -p /mnt/home
	echo "slave home directory file!" > /mnt/home/hello.txt
	echo "slave root directory file!" > /mnt/hello.txt
	sudo umount /mnt
	sudo losetup -d /dev/loop0

.PHONY: mount0
mount0: $(BUILD)/master.img
	sudo losetup /dev/loop0 --partscan $<
	sudo mount /dev/loop0p1 /mnt
	sudo chown $(USER) /mnt

.PHONY: umount0
umount0: /dev/loop0
	-sudo umount /mnt
	-sudo losetup -d $<

.PHONY: mount1
mount1: $(BUILD)/slave.img
	sudo losetup /dev/loop0 --partscan $<
	sudo mount /dev/loop0p1 /mnt
	sudo chown $(USER) /mnt

.PHONY: umount1
umount1: /dev/loop0
	-sudo umount /mnt
	-sudo losetup -d $<


IMAGE:=$(BUILD)/master.img $(BUILD)/slave.img

.PHONY: image
image: $(IMAGE)

