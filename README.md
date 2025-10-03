# Snow Kernel
A tiny OS (well a kernel) that has a few commands. Inspired from DOS. Built with modern standards, or at least tries to achieve that.


## Build

you need these:
- nasm
- gcc or i686-elf-gcc
- ld or i686-elf-ld
- grub-mkrescue (from grub and xorriso packages)
- qemu-system-i386

```
make iso
make run
```

The filesystem is RAM-only and resets on reboot.
