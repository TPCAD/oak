# architecture
ARCH:=x86

# source directories
SRC:=.
ARCH_DIR:=$(SRC)/arch/$(ARCH)
KERNEL_DIR:=$(SRC)/kernel
BOOT_DIR:=$(ARCH_DIR)/boot
LIB_DIR:=$(SRC)/lib
BUILTIN_DIR:=$(SRC)/builtin
UTILS_DIR:=$(SRC)/utils

# build directories
BUILD:=../build
BUILD_BOOT:=$(BUILD)/boot
BUILD_KERNEL:=$(BUILD)/kernel
BUILD_BUILTIN:=$(BUILD)/builtin
BUILD_LIB:=$(BUILD)/lib

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
CFLAGS+= -DOAK
CFLAGS:=$(strip ${CFLAGS})

include makefiles/compile.mk

include makefiles/images.mk
include makefiles/emulators.mk

.PHONY: clean
clean:
	rm -rf $(BUILD)
