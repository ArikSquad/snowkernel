#pragma once
#include <stdint.h>

enum
{
    SYS_read = 0,
    SYS_write = 1,
    SYS_open = 2,
    SYS_close = 3,
    SYS_stat = 4,
    SYS_fstat = 5,
    SYS_lseek = 8,
    SYS_fork = 57,
    SYS_execve = 59,
};

long sys_read(int fd, void *buf, unsigned long count);
long sys_write(int fd, const void *buf, unsigned long count);
long sys_open(const char *path, int flags, int mode);
long sys_close(int fd);
long sys_stat(const char *path, void *ubuf);
long sys_fstat(int fd, void *ubuf);
long sys_lseek(int fd, long off, int whence);
long sys_fork(void);
long sys_execve(const char *path, char *const argv[], char *const envp[]);
long syscall_dispatch(long num, long a1, long a2, long a3, long a4, long a5, long a6);
