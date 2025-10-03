# Snow Kernel
A tiny OS (well a kernel) that has a few commands. Inspired from DOS. Built with modern standards, or at least tries to achieve that.


## Build

you need these for x86_32:
- nasm
- gcc or i686-elf-gcc
- ld or i686-elf-ld
- grub-mkrescue (from grub and xorriso packages)
- qemu-system-i386

```
make iso
make run
```

you can also run it on x86_64 with `make run ARCH=x86_64`
