# User Programs

This directory contains userland test programs for SnowKernel.

## Programs

### hello.c
Basic "Hello World" program demonstrating:
- `printf()` output
- Direct `write()` syscall

### syscall_test.c
Comprehensive syscall test program that exercises:
- Process identification (getpid, getppid)
- User/group IDs (getuid, getgid, geteuid, getegid)
- Directory operations (getcwd, chdir, mkdir)
- File operations (access, open, read, close)
- /proc filesystem reading
- Time functions (time, gettimeofday)
- Permission management (umask)

### newlib_demo.c
Demonstrates newlib C library integration:
- Standard I/O functions (printf, fprintf, puts)
- String operations (strcpy, strcat, strlen)
- Dynamic memory (malloc, free)
- File I/O (fopen, fgets, fclose)
- POSIX functions (getcwd, stat)
- Process information

## Building

User programs are built with newlib and linked statically:

```bash
make newlib        # Build newlib (one-time setup)
make userprogs     # Build user programs
```

## Runtime

User programs are embedded into the kernel image and can be executed from the shell:

```
> exec /bin/hello_user.elf
```

Or from the kernel:

```c
sys_execve("/bin/hello_user.elf", NULL, NULL);
```

## Adding New Programs

1. Create a new `.c` file in `src/user/`
2. Update `USER_SRC` in the Makefile to include your program
3. Optionally create a separate `.elf` target for the program
4. Rebuild with `make userprogs`

## Syscall Usage

User programs can use syscalls via:

1. **Newlib wrappers** (recommended):
   ```c
   #include <unistd.h>
   int fd = open("/proc/meminfo", O_RDONLY);
   ```

2. **Direct syscall** (low-level):
   ```c
   asm volatile("int $0x80" : ...);
   ```

The syscall interface uses `int 0x80` with Linux-like syscall numbers (see `include/syscall.h`).

## CRT0

`crt0_user.S` provides the C runtime startup code:
- Sets up the stack
- Calls `main(argc, argv)`
- Handles return value via `exit()`

## Memory Layout

User programs run in user mode with:
- Code/data loaded at their specified virtual addresses (from ELF)
- Stack at high address (~0x7FFFFFF0000)
- Heap managed by `brk` syscall (used by malloc)

## Limitations

- No dynamic linking (static binaries only)
- No environment variables yet
- No signal handling
- Limited file descriptors (32 max per process)
- No fork/exec (execve replaces current process)
