# SnowKernel System Calls

This document describes the system calls implemented in SnowKernel.

## Overview

SnowKernel implements a subset of POSIX system calls to provide compatibility with standard C libraries like newlib. System calls are invoked via the `int 0x80` interrupt mechanism on x86-64.

## Implemented System Calls

### File I/O Operations

#### `SYS_read (0)`
```c
long sys_read(int fd, void *buf, unsigned long count);
```
Read data from a file descriptor.

#### `SYS_write (1)`
```c
long sys_write(int fd, const void *buf, unsigned long count);
```
Write data to a file descriptor.

#### `SYS_open (2)`
```c
long sys_open(const char *path, int flags, int mode);
```
Open a file and return a file descriptor.

#### `SYS_close (3)`
```c
long sys_close(int fd);
```
Close a file descriptor.

#### `SYS_lseek (8)`
```c
long sys_lseek(int fd, long off, int whence);
```
Reposition file offset.

### File Status Operations

#### `SYS_stat (4)`
```c
long sys_stat(const char *path, void *ubuf);
```
Get file status by path.

#### `SYS_fstat (5)`
```c
long sys_fstat(int fd, void *ubuf);
```
Get file status by file descriptor.

#### `SYS_access (21)`
```c
long sys_access(const char *path, int mode);
```
Check file accessibility.

### Directory Operations

#### `SYS_chdir (80)`
```c
long sys_chdir(const char *path);
```
Change current working directory.

#### `SYS_getcwd (79)`
```c
long sys_getcwd(char *buf, unsigned long size);
```
Get current working directory.

#### `SYS_mkdir (83)`
```c
long sys_mkdir(const char *path, int mode);
```
Create a directory.

#### `SYS_rmdir (84)`
```c
long sys_rmdir(const char *path);
```
Remove a directory.

### File Operations

#### `SYS_unlink (87)`
```c
long sys_unlink(const char *path);
```
Remove a file.

#### `SYS_rename (82)`
```c
long sys_rename(const char *oldpath, const char *newpath);
```
Rename a file or directory.

### File Descriptor Operations

#### `SYS_dup (32)`
```c
long sys_dup(int oldfd);
```
Duplicate a file descriptor.

#### `SYS_dup2 (33)`
```c
long sys_dup2(int oldfd, int newfd);
```
Duplicate a file descriptor to a specific number.

#### `SYS_fcntl (72)`
```c
long sys_fcntl(int fd, int cmd, long arg);
```
File control operations.

#### `SYS_ioctl (16)`
```c
long sys_ioctl(int fd, unsigned long request, unsigned long arg);
```
Device control operations (stub).

### Process Operations

#### `SYS_getpid (39)`
```c
long sys_getpid(void);
```
Get process ID.

#### `SYS_getppid (110)`
```c
long sys_getppid(void);
```
Get parent process ID.

#### `SYS_fork (57)`
```c
long sys_fork(void);
```
Create a child process (not yet implemented).

#### `SYS_execve (59)`
```c
long sys_execve(const char *path, char *const argv[], char *const envp[]);
```
Execute a program.

#### `SYS_exit (60)`
```c
long sys_exit(int code);
```
Terminate the calling process.

#### `SYS_waitpid (61)`
```c
long sys_waitpid(int pid, int *status, int options);
```
Wait for a child process (stub).

### User/Group ID Operations

#### `SYS_getuid (102)`
```c
long sys_getuid(void);
```
Get real user ID (always returns 0).

#### `SYS_getgid (104)`
```c
long sys_getgid(void);
```
Get real group ID (always returns 0).

#### `SYS_geteuid (107)`
```c
long sys_geteuid(void);
```
Get effective user ID (always returns 0).

#### `SYS_getegid (108)`
```c
long sys_getegid(void);
```
Get effective group ID (always returns 0).

### Memory Management

#### `SYS_brk (12)`
```c
long sys_brk(void *addr);
```
Change data segment size.

### Permissions

#### `SYS_umask (95)`
```c
long sys_umask(int mask);
```
Set file creation mask (returns default 0022).

### Time Operations

#### `SYS_time (201)`
```c
long sys_time(long *tloc);
```
Get time in seconds since epoch (simplified, returns 0).

#### `SYS_gettimeofday (96)`
```c
long sys_gettimeofday(void *tv, void *tz);
```
Get time of day (simplified, returns 0).

### Pipe Operations

#### `SYS_pipe (22)`
```c
long sys_pipe(int *pipefd);
```
Create a pipe (not yet implemented).

### Directory Reading

#### `SYS_readdir (89)`
```c
long sys_readdir(int fd, void *dirp, unsigned int count);
```
Read directory entries (not yet implemented).

## /proc Filesystem

SnowKernel implements a basic `/proc` filesystem for process and system information:

### `/proc/meminfo`
Memory information including total and free memory.

### `/proc/cpuinfo`
CPU information including vendor, model, and features.

### `/proc/version`
Kernel version information.

### `/proc/self/`
Current process information:
- `cmdline` - Command line
- `status` - Process status

### `/proc/1/`
Init process (PID 1) information:
- `cmdline` - Command line
- `status` - Process status

## Newlib Integration

All system calls are properly wrapped in `src/libc/syscalls.c` to provide compatibility with newlib. The wrappers use weak symbol aliases to allow overriding.

### Example Usage

```c
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("My PID: %d\n", getpid());
    printf("My parent PID: %d\n", getppid());
    
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("Current directory: %s\n", cwd);
    
    return 0;
}
```

## Implementation Notes

1. **Error Handling**: Most syscalls return -1 on error. Proper errno support is minimal.

2. **User/Group IDs**: All UID/GID syscalls return 0 (root) as there is no user management yet.

3. **Time**: Time syscalls return 0 as there is no real-time clock implementation yet.

4. **Stubs**: Some syscalls like `fork`, `pipe`, `waitpid`, and `readdir` are stubs that return -1 (not implemented).

5. **Security**: There is no permission checking yet - all operations succeed if the file/directory exists.

6. **Path Resolution**: Path resolution is basic and doesn't fully handle all edge cases (e.g., `..` and `.` components).

## Future Work

- Implement `fork` and `pipe` syscalls
- Add proper directory reading with `readdir`/`getdents`
- Implement signal handling
- Add memory mapping (`mmap`/`munmap`)
- Implement real-time clock support
- Add proper permission and security checks
- Expand `/proc` filesystem with more entries
