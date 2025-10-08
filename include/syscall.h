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
    SYS_brk = 12,
    SYS_ioctl = 16,
    SYS_access = 21,
    SYS_pipe = 22,
    SYS_dup = 32,
    SYS_dup2 = 33,
    SYS_fork = 57,
    SYS_execve = 59,
    SYS_exit = 60,
    SYS_waitpid = 61,
    SYS_fcntl = 72,
    SYS_getcwd = 79,
    SYS_chdir = 80,
    SYS_rename = 82,
    SYS_mkdir = 83,
    SYS_rmdir = 84,
    SYS_unlink = 87,
    SYS_readdir = 89,
    SYS_umask = 95,
    SYS_gettimeofday = 96,
    SYS_getuid = 102,
    SYS_getgid = 104,
    SYS_geteuid = 107,
    SYS_getegid = 108,
    SYS_getpid = 39,
    SYS_getppid = 110,
    SYS_time = 201,
};

long sys_read(int fd, void *buf, unsigned long count);
long sys_write(int fd, const void *buf, unsigned long count);
long sys_open(const char *path, int flags, int mode);
long sys_close(int fd);
long sys_stat(const char *path, void *ubuf);
long sys_fstat(int fd, void *ubuf);
long sys_lseek(int fd, long off, int whence);
long sys_pipe(int *pipefd);
long sys_dup(int oldfd);
long sys_dup2(int oldfd, int newfd);
long sys_fork(void);
long sys_execve(const char *path, char *const argv[], char *const envp[]);
long sys_exit(int code);
long sys_brk(void *addr);
long sys_getpid(void);
long sys_getppid(void);
long sys_getuid(void);
long sys_getgid(void);
long sys_geteuid(void);
long sys_getegid(void);
long sys_chdir(const char *path);
long sys_getcwd(char *buf, unsigned long size);
long sys_mkdir(const char *path, int mode);
long sys_rmdir(const char *path);
long sys_unlink(const char *path);
long sys_rename(const char *oldpath, const char *newpath);
long sys_access(const char *path, int mode);
long sys_readdir(int fd, void *dirp, unsigned int count);
long sys_fcntl(int fd, int cmd, long arg);
long sys_ioctl(int fd, unsigned long request, unsigned long arg);
long sys_umask(int mask);
long sys_time(long *tloc);
long sys_gettimeofday(void *tv, void *tz);
long sys_waitpid(int pid, int *status, int options);
long syscall_dispatch(long num, long a1, long a2, long a3, long a4, long a5, long a6);
