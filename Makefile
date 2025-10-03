TARGET = kernel.bin
ISO = snowkernel.iso

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

CFLAGS = -ffreestanding -O0 -Wall -Wextra -std=gnu11 -m32 -fno-pic -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables -Iinclude
LDFLAGS = -T linker.ld -m elf_i386

SRC_C = \
	src/kernel/kprint.c \
	src/kernel/string.c \
	src/kernel/keymap.c \
	src/kernel/keyboard.c \
	src/kernel/serial.c \
	src/fs/fs.c \
	src/kernel/shell.c \
	src/kernel/kernel.c

SRC_ASM = \
	src/boot/multiboot_header.asm

OBJ = $(SRC_C:.c=.o) $(SRC_ASM:.asm=.o)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	@command -v nasm >/dev/null 2>&1 || { \
	  echo "Error: nasm not found. Install it (e.g. pacman -S nasm)"; \
	  exit 127; \
	}
	nasm -f elf32 $< -o $@

$(TARGET): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

iso: $(TARGET)
	mkdir -p build/iso/boot/grub
	cp $(TARGET) build/iso/boot/kernel.bin
	cp grub/grub.cfg build/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) build/iso

QEMU_FLAGS ?= -serial stdio -no-reboot -no-shutdown

run: iso
	qemu-system-i386 -cdrom $(ISO) $(QEMU_FLAGS)

run-kernel: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -serial stdio -nographic -no-reboot -no-shutdown

debug-run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET) -serial stdio -nographic -no-reboot -no-shutdown

clean:
	rm -f $(OBJ) $(TARGET) $(ISO)
	rm -rf build/iso

.PHONY: all iso run run-kernel clean
