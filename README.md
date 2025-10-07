# Snow Kernel
A tiny OS (well a kernel) that has a few commands. Inspired from DOS. Built with modern standards, or at least tries to achieve that.


## Build

You need these at least:
- nasm
- x86_64-elf-gcc (or a compatible cross-compiler)
- x86_64-elf-ld (or a compatible linker)
- grub-mkrescue (from grub and xorriso packages)
- qemu-system-x86_64

Build and run:

```
make newlib
make userprogs
make iso
make run
```
