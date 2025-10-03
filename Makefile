TARGET = kernel.bin
ISO = yapkernel.iso
CC ?= i686-elf-gcc
LD ?= i686-elf-ld
AS = nasm
CFLAGS = -ffreestanding -O2 -Wall -Wextra -std=gnu11 -m32 -fno-pic -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-exceptions
LDFLAGS = -T linker.ld -m elf_i386

SRC_C = \
  src/kernel/kprint.c \
  src/kernel/string.c \
  src/kernel/keyboard.c \
  src/fs/fs.c \
  src/kernel/shell.c \
  src/kernel/kernel.c

SRC_ASM = src/boot/multiboot_header.asm

OBJ = $(SRC_C:.c=.o) $(SRC_ASM:.asm=.o)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm -f elf32 $< -o $@

$(TARGET): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

iso: $(TARGET)
	mkdir -p build/iso/boot/grub
	cp $(TARGET) build/iso/boot/kernel.bin
	cp grub/grub.cfg build/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) build/iso

run: iso
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -f $(OBJ) $(TARGET) $(ISO)
	rm -rf build/iso
