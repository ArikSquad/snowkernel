// this is a minimal version now due to newlib doing stuff
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>
#include "../kernel/kprint.h"
#include "../fs/fs.h"

int errno; // weak errno

static inline long ksys(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = a3;
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    register long r9 __asm__("r9") = a6;
    __asm__ volatile("int $0x80" : "+r"(rax) : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return rax;
}

void *_sbrk(ptrdiff_t incr)
{
    long cur = ksys(SYS_brk, 0, 0, 0, 0, 0, 0);
    if (cur < 0)
        return (void *)-1;
    long want = cur + incr;
    long res = ksys(SYS_brk, want, 0, 0, 0, 0, 0);
    if (res < 0)
        return (void *)-1;
    return (void *)cur;
}

int _close(int fd)
{
    long r = ksys(SYS_close, fd, 0, 0, 0, 0, 0);
    return (r < 0 ? -1 : 0);
}

int _fstat(int fd, void *st)
{
    long r = ksys(SYS_fstat, fd, (long)st, 0, 0, 0, 0);
    return (r < 0 ? -1 : 0);
}

int _isatty(int fd)
{
    struct mini_stat
    {
        uint64_t dev, ino;
        uint32_t mode;
        uint32_t nlink;
        uint32_t uid, gid;
        uint64_t rdev, size, blksize, blocks, at, mt, ct;
    } st;
    if (_fstat(fd, &st) == 0)
    {
        if (fd < 3)
            return 1;
        if ((st.mode & 0020000) == 0020000)
            return 1; // S_IFCHR
    }
    return 0;
}

long _lseek(int fd, long off, int whence)
{
    long r = ksys(SYS_lseek, fd, off, whence, 0, 0, 0);
    return r;
}

int _read(int fd, void *buf, size_t cnt)
{
    long r = ksys(SYS_read, fd, (long)buf, cnt, 0, 0, 0);
    return (int)r;
}
int _write(int fd, const void *buf, size_t cnt)
{
    long r = ksys(SYS_write, fd, (long)buf, cnt, 0, 0, 0);
    return (int)r;
}
void _exit(int code)
{
    ksys(SYS_exit, code, 0, 0, 0, 0, 0);
    for (;;)
        __asm__("hlt");
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = 0;
    return -1;
}
int _getpid(void) { return 1; }

#ifdef __GNUC__
int write(int fd, const void *buf, size_t cnt) __attribute__((weak, alias("_write")));
int read(int fd, void *buf, size_t cnt) __attribute__((weak, alias("_read")));
int close(int fd) __attribute__((weak, alias("_close")));
int fstat(int fd, void *st) __attribute__((weak, alias("_fstat")));
int isatty(int fd) __attribute__((weak, alias("_isatty")));
long lseek(int fd, long off, int whence) __attribute__((weak, alias("_lseek")));
void *sbrk(ptrdiff_t inc) __attribute__((weak, alias("_sbrk")));
int kill(int pid, int sig) __attribute__((weak, alias("_kill")));
int getpid(void) __attribute__((weak, alias("_getpid")));
#else
int write(int fd, const void *buf, size_t cnt) { return _write(fd, buf, cnt); }
int read(int fd, void *buf, size_t cnt) { return _read(fd, buf, cnt); }
int close(int fd) { return _close(fd); }
int fstat(int fd, void *st) { return _fstat(fd, st); }
int isatty(int fd) { return _isatty(fd); }
long lseek(int fd, long off, int whence) { return _lseek(fd, off, whence); }
void *sbrk(ptrdiff_t inc) { return _sbrk(inc); }
int kill(int pid, int sig) { return _kill(pid, sig); }
int getpid(void) { return _getpid(); }
#endif
