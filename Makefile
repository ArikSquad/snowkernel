ARCH := x86_64
FS_BACKEND ?= mem
TARGET = kernel.bin
ISO = snowkernel.iso
DISK_IMG = disk.img

CC := x86_64-elf-gcc
LD := x86_64-elf-ld
ASMFORMAT := elf64

CFLAGS = -ffreestanding -O0 -Wall -Wextra -std=gnu11 -m64 -fno-pic -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables -Iinclude
LDFLAGS = -T linker64.ld -m elf_x86_64

SRC_KERNEL_C := $(wildcard src/kernel/*.c)
SRC_KERNEL_ASM := $(wildcard src/kernel/*.S)
SRC_CPU_C    := $(filter-out src/cpu/idt.c src/cpu/irq.c src/cpu/power.c,$(wildcard src/cpu/*.c))
SRC_DRIVERS_C:= $(wildcard src/drivers/*.c)
SRC_LIBC_C   := $(wildcard src/libc/*.c)

SRC_C_COMMON = $(SRC_KERNEL_C) $(SRC_CPU_C) $(SRC_DRIVERS_C) $(SRC_LIBC_C)

COREUTILS_DIR = lib/coreutils
COREUTILS_PACKAGES = uu_ls uu_cat uu_echo uu_mkdir uu_rm uu_cp uu_mv uu_pwd uu_touch uu_chmod uu_chown uu_ln uu_df uu_base32 uu_du uu_head uu_tail uu_sort uu_uniq uu_wc uu_cut uu_paste uu_tr uu_shred uu_dd uu_od uu_stat uu_readlink uu_basename uu_dirname uu_realpath uu_mktemp uu_seq uu_factor uu_numfmt uu_shuf uu_tac uu_nl uu_pr uu_fmt uu_fold uu_expand uu_unexpand uu_yes uu_false uu_true uu_test uu_expr uu_date uu_sleep uu_timeout uu_nice uu_nohup uu_printenv uu_printf uu_pathchk uu_tty uu_whoami uu_id uu_groups uu_logname uu_users uu_who uu_uptime uu_hostname uu_uname uu_arch uu_nproc uu_sync
COREUTILS_CMDS = $(COREUTILS_PACKAGES:uu_%=%)
EMBED_OBJS = $(foreach cmd, $(COREUTILS_CMDS), src/kernel/corebin_$(cmd).o)

src/kernel/corebin_%.o: $(COREUTILS_DIR)/target/release/%
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

OBJ_EXTRA = $(EMBED_OBJS)

HELLO_BIN_OBJ = src/kernel/corebin_hello.o
$(HELLO_BIN_OBJ): $(USER_ELF)
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $(USER_ELF) $@

OBJ_EXTRA += $(HELLO_BIN_OBJ)

# User programs
NEWLIB_SRC = lib/newlib
NEWLIB_BUILD = build/newlib
NEWLIB_TARGET = x86_64-elf
NEWLIB_LIBC = $(NEWLIB_BUILD)/$(NEWLIB_TARGET)/newlib/libc.a
NEWLIB_LIBM = $(NEWLIB_BUILD)/$(NEWLIB_TARGET)/newlib/libm.a

USER_CFLAGS = -O0 -Wall -Wextra -std=gnu11 -m64 -ffreestanding -fno-stack-protector -fno-pic -nostdlib \
	-I$(NEWLIB_BUILD)/$(NEWLIB_TARGET)/newlib/targ-include -I$(NEWLIB_SRC)/newlib/libc/include -Iinclude \
	-D__NEWLIB_USER__
USER_SRC = src/user/hello.c src/libc/syscalls.c
USER_ASM = src/user/crt0_user.S
USER_OBJS = $(USER_ASM:.S=.u_o) $(patsubst %.c,%.u_o,$(USER_SRC))
USER_ELF = build/hello_user.elf

$(USER_ASM:.S=.u_o): $(USER_ASM) $(ARCH_STAMP) | newlib
	$(CC) $(USER_CFLAGS) -c $< -o $@
%.u_o: %.c $(ARCH_STAMP) | newlib
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_ELF): $(USER_OBJS) linker_user.ld $(NEWLIB_LIBC)
	$(LD) -T linker_user.ld -o $@ $(USER_OBJS) $(NEWLIB_LIBC) $(NEWLIB_LIBM)
	@echo "[user] linked $@ with newlib"

userprogs: $(USER_ELF)
	@echo "[user] done"

.PHONY: newlib
newlib: $(NEWLIB_LIBC)
	@echo "[newlib] ready"

$(NEWLIB_LIBC):
	@echo "[newlib] configuring/building (one-time)"
	mkdir -p $(NEWLIB_BUILD)
	@if [ ! -f $(NEWLIB_BUILD)/Makefile ]; then \
	  echo "int _start(void){return 0;}" > $(NEWLIB_BUILD)/dummycrt0.c; \
	  $(CC) -c -ffreestanding -nostdlib -o $(NEWLIB_BUILD)/crt0.o $(NEWLIB_BUILD)/dummycrt0.c; \
	  (cd $(NEWLIB_BUILD) && \
	    CC=gcc CFLAGS_FOR_TARGET="-O0 -g" \
	    ../../$(NEWLIB_SRC)/configure --target=$(NEWLIB_TARGET) --disable-multilib --disable-nls --disable-shared --disable-newlib-supplied-syscalls); \
	fi
	$(MAKE) -C $(NEWLIB_BUILD) CFLAGS_FOR_TARGET="-O0 -g" all-target-newlib

.PHONY: build_coreutils
build_coreutils:
	cd $(COREUTILS_DIR) && RUSTFLAGS="-C target-feature=+crt-static -C relocation-model=static -C panic=abort" cargo build --release --target x86_64-unknown-linux-musl $(foreach pkg, $(COREUTILS_PACKAGES), -p $(pkg))

ifeq ($(FS_BACKEND),disk)
  SRC_C_COMMON += src/fs/fs_disk.c
  CFLAGS += -DFS_BACKEND_DISK
else
  SRC_C_COMMON += src/fs/fs.c
  CFLAGS += -DFS_BACKEND_MEM
endif

SRC_C = $(SRC_C_COMMON)
SRC_ASM = src/boot/multiboot64.asm src/cpu/int80_64.asm src/kernel/enter_user_64.asm src/cpu/irq_stubs.asm src/kernel/context.S

# Ensure multiboot header object is first in final link (required by GRUB within first 8KiB)
# Place libcorebins.a at the end so the linker can extract members for symbols
# referenced by later object files (e.g. src/kernel/kernel64.o).
# Convert both .asm and .S assembly sources into .o objects
OBJ_ASM = $(patsubst %.asm,%.o,$(filter %.asm,$(SRC_ASM))) $(patsubst %.S,%.o,$(filter %.S,$(SRC_ASM)))
OBJ = $(OBJ_ASM) $(patsubst src/cpu/idt.c,,$(patsubst src/cpu/irq.c,,$(patsubst src/cpu/power.c,,$(SRC_C:.c=.o)))) $(OBJ_EXTRA)

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

# Also support .S assembly files (use the C compiler/assembler (gas) which
# handles AT&T-syntax .S sources and preprocessing). This avoids nasm parse
# errors for .S files written in gas/AT&T syntax.
%.o: %.S $(ARCH_STAMP)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c $(ARCH_STAMP)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	# Link using the deduplicated object list. Avoid passing wildcard patterns that
	# cause each object to appear multiple times (which produced multiple definition errors).
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
	rm -f $(OBJ) $(TARGET) $(ISO) libcorebins.a
	rm -rf build/iso

.PHONY: all iso run run-kernel clean
