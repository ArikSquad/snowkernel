# Snow Kernel
A tiny OS (well a kernel) that has a few commands. Inspired from DOS. Built with modern standards, or at least tries to achieve that.

## Features

- **POSIX-like System Calls**: Implements many standard POSIX syscalls for file I/O, process management, directory operations, and more
- **/proc Filesystem**: Basic `/proc` filesystem for system information (`/proc/meminfo`, `/proc/cpuinfo`, etc.)
- **Newlib Integration**: Full integration with newlib C library for userland applications
- **x86-64 Architecture**: 64-bit kernel with support for user-mode programs
- **Virtual Filesystem**: In-memory filesystem with support for files, directories, and character devices
- **TTY Support**: Terminal I/O with `/dev/tty0`

For details on available system calls, see [SYSCALLS.md](SYSCALLS.md).

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

Last commit with 32-bit support is 2e4d00061fed1183adb9f77e681e242306fca6b6