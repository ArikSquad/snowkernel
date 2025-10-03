ARCH ?= i386
FS_BACKEND ?= mem
TARGET = kernel.bin
ISO = snowkernel.iso
DISK_IMG = disk.img

ifeq ($(ARCH),x86_64)
	CC := x86_64-elf-gcc
	LD := x86_64-elf-ld
    ASMFORMAT := elf64
else
	ifeq ($(shell command -v i686-elf-gcc >/dev/null 2>&1 && echo yes),yes)
		CC := i686-elf-gcc
	else
		CC := gcc
	endif
	ifeq ($(shell command -v i686-elf-ld >/dev/null 2>&1 && echo yes),yes)
		LD := i686-elf-ld
	else
		LD := ld
	endif
    ASMFORMAT := elf32
endif

ifeq ($(ARCH),x86_64)
	CFLAGS = -ffreestanding -O0 -Wall -Wextra -std=gnu11 -m64 -fno-pic -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables -Iinclude
	LDFLAGS = -T linker64.ld -m elf_x86_64
else
	CFLAGS = -ffreestanding -O0 -Wall -Wextra -std=gnu11 -m32 -fno-pic -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables -Iinclude
	LDFLAGS = -T linker.ld -m elf_i386
endif


SRC_C_COMMON = \
	src/kernel/kprint.c \
	src/kernel/string.c \
	src/kernel/keymap.c \
	src/kernel/keyboard.c \
	src/kernel/serial.c \
	src/kernel/env.c \
	src/kernel/ata.c \
	src/kernel/shell.c \
	src/kernel/kernel.c \
	src/kernel/syscall.c \
	src/kernel/idt.c \
	src/kernel/pic.c \
	src/kernel/irq.c \
	src/kernel/irq_kbd.c \
	src/kernel/irq_timer.c

ifeq ($(FS_BACKEND),disk)
  SRC_C_COMMON += src/fs/fs_disk.c
  CFLAGS += -DFS_BACKEND_DISK
else
  SRC_C_COMMON += src/fs/fs.c
  CFLAGS += -DFS_BACKEND_MEM
endif

ifeq ($(ARCH),x86_64)
	SRC_C_COMMON += src/kernel/kernel64.c
endif
SRC_C_COMMON += src/kernel/int80.c

SRC_C = $(SRC_C_COMMON)

ifeq ($(ARCH),x86_64)
	SRC_ASM = src/boot/multiboot64.asm src/kernel/int80_64.asm src/kernel/irq_stubs.asm
else
	SRC_ASM = src/boot/multiboot_header.asm src/kernel/idt_int80.asm src/kernel/irq_stubs32.asm
endif

OBJ = $(SRC_C:.c=.o) $(SRC_ASM:.asm=.o)

all: $(TARGET)

ARCH_STAMP = .arch_$(ARCH)

$(ARCH_STAMP):
	@rm -f .arch_* 2>/dev/null || true
	@touch $@

%.o: %.asm $(ARCH_STAMP)
	@command -v nasm >/dev/null 2>&1 || { \
	  echo "Error: nasm not found. Install it (e.g. pacman -S nasm)"; \
	  exit 127; \
	}
	nasm -f $(ASMFORMAT) $< -o $@

%.o: %.c $(ARCH_STAMP)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

iso: $(TARGET) $(DISK_IMG)
	mkdir -p build/iso/boot/grub
	cp $(TARGET) build/iso/boot/kernel.bin
	cp grub/grub.cfg build/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) build/iso

QEMU_FLAGS ?= -serial stdio -no-reboot -no-shutdown

run: iso
	qemu-system-$(ARCH) -cdrom $(ISO) -drive file=$(DISK_IMG),format=raw,if=ide $(QEMU_FLAGS)

run-kernel: $(TARGET) $(DISK_IMG)
	qemu-system-$(ARCH) -kernel $(TARGET) -serial stdio -nographic -drive file=$(DISK_IMG),format=raw,if=ide -no-reboot -no-shutdown

debug-run: $(TARGET)
	qemu-system-$(ARCH) -kernel $(TARGET) -serial stdio -nographic -drive file=$(DISK_IMG),format=raw,if=ide -no-reboot -no-shutdown

$(DISK_IMG):
	@[ -f $(DISK_IMG) ] || dd if=/dev/zero of=$(DISK_IMG) bs=1M count=8 2>/dev/null

clean:
	rm -f $(OBJ) $(TARGET) $(ISO)
	rm -rf build/iso

.PHONY: all iso run run-kernel clean
