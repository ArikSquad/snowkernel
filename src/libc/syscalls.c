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
int _getpid(void) { 
    long r = ksys(SYS_getpid, 0, 0, 0, 0, 0, 0);
    return (int)r;
}

/* New syscall wrappers */
int _getppid(void) {
    long r = ksys(SYS_getppid, 0, 0, 0, 0, 0, 0);
    return (int)r;
}

int _getuid(void) {
    long r = ksys(SYS_getuid, 0, 0, 0, 0, 0, 0);
    return (int)r;
}

int _getgid(void) {
    long r = ksys(SYS_getgid, 0, 0, 0, 0, 0, 0);
    return (int)r;
}

int _geteuid(void) {
    long r = ksys(SYS_geteuid, 0, 0, 0, 0, 0, 0);
    return (int)r;
}

int _getegid(void) {
    long r = ksys(SYS_getegid, 0, 0, 0, 0, 0, 0);
    return (int)r;
}

int _chdir(const char *path) {
    long r = ksys(SYS_chdir, (long)path, 0, 0, 0, 0, 0);
    return (int)r;
}

char *_getcwd(char *buf, size_t size) {
    long r = ksys(SYS_getcwd, (long)buf, size, 0, 0, 0, 0);
    return (char *)r;
}

int _mkdir(const char *path, int mode) {
    long r = ksys(SYS_mkdir, (long)path, mode, 0, 0, 0, 0);
    return (int)r;
}

int _rmdir(const char *path) {
    long r = ksys(SYS_rmdir, (long)path, 0, 0, 0, 0, 0);
    return (int)r;
}

int _unlink(const char *path) {
    long r = ksys(SYS_unlink, (long)path, 0, 0, 0, 0, 0);
    return (int)r;
}

int _rename(const char *old, const char *new) {
    long r = ksys(SYS_rename, (long)old, (long)new, 0, 0, 0, 0);
    return (int)r;
}

int _access(const char *path, int mode) {
    long r = ksys(SYS_access, (long)path, mode, 0, 0, 0, 0);
    return (int)r;
}

int _fcntl(int fd, int cmd, long arg) {
    long r = ksys(SYS_fcntl, fd, cmd, arg, 0, 0, 0);
    return (int)r;
}

int _ioctl(int fd, unsigned long request, unsigned long arg) {
    long r = ksys(SYS_ioctl, fd, request, arg, 0, 0, 0);
    return (int)r;
}

int _umask(int mask) {
    long r = ksys(SYS_umask, mask, 0, 0, 0, 0, 0);
    return (int)r;
}

long _time(long *tloc) {
    long r = ksys(SYS_time, (long)tloc, 0, 0, 0, 0, 0);
    return r;
}

int _gettimeofday(void *tv, void *tz) {
    long r = ksys(SYS_gettimeofday, (long)tv, (long)tz, 0, 0, 0, 0);
    return (int)r;
}

int _waitpid(int pid, int *status, int options) {
    long r = ksys(SYS_waitpid, pid, (long)status, options, 0, 0, 0);
    return (int)r;
}

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
int getppid(void) __attribute__((weak, alias("_getppid")));
int getuid(void) __attribute__((weak, alias("_getuid")));
int getgid(void) __attribute__((weak, alias("_getgid")));
int geteuid(void) __attribute__((weak, alias("_geteuid")));
int getegid(void) __attribute__((weak, alias("_getegid")));
int chdir(const char *path) __attribute__((weak, alias("_chdir")));
char *getcwd(char *buf, size_t size) __attribute__((weak, alias("_getcwd")));
int mkdir(const char *path, int mode) __attribute__((weak, alias("_mkdir")));
int rmdir(const char *path) __attribute__((weak, alias("_rmdir")));
int unlink(const char *path) __attribute__((weak, alias("_unlink")));
int rename(const char *old, const char *new) __attribute__((weak, alias("_rename")));
int access(const char *path, int mode) __attribute__((weak, alias("_access")));
int fcntl(int fd, int cmd, long arg) __attribute__((weak, alias("_fcntl")));
int ioctl(int fd, unsigned long request, unsigned long arg) __attribute__((weak, alias("_ioctl")));
int umask(int mask) __attribute__((weak, alias("_umask")));
long time(long *tloc) __attribute__((weak, alias("_time")));
int gettimeofday(void *tv, void *tz) __attribute__((weak, alias("_gettimeofday")));
int waitpid(int pid, int *status, int options) __attribute__((weak, alias("_waitpid")));
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
int getppid(void) { return _getppid(); }
int getuid(void) { return _getuid(); }
int getgid(void) { return _getgid(); }
int geteuid(void) { return _geteuid(); }
int getegid(void) { return _getegid(); }
int chdir(const char *path) { return _chdir(path); }
char *getcwd(char *buf, size_t size) { return _getcwd(buf, size); }
int mkdir(const char *path, int mode) { return _mkdir(path, mode); }
int rmdir(const char *path) { return _rmdir(path); }
int unlink(const char *path) { return _unlink(path); }
int rename(const char *old, const char *new) { return _rename(old, new); }
int access(const char *path, int mode) { return _access(path, mode); }
int fcntl(int fd, int cmd, long arg) { return _fcntl(fd, cmd, arg); }
int ioctl(int fd, unsigned long request, unsigned long arg) { return _ioctl(fd, request, arg); }
int umask(int mask) { return _umask(mask); }
long time(long *tloc) { return _time(tloc); }
int gettimeofday(void *tv, void *tz) { return _gettimeofday(tv, tz); }
int waitpid(int pid, int *status, int options) { return _waitpid(pid, status, options); }
#endif
