#include "syscall.h"
#include "kprint.h"
#include "../fs/fs.h"
#include "string.h"
#include "stat.h"

typedef struct
{
    int used;
    node_t *node;
    size_t ofs;
    int flags;
} fd_entry_t;
#define FD_MAX 32
static fd_entry_t fdtab[FD_MAX];
static int alloc_fd(node_t *n)
{
    for (int i = 3; i < FD_MAX; i++)
    {
        if (!fdtab[i].used)
        {
            fdtab[i].used = 1;
            fdtab[i].node = n;
            fdtab[i].ofs = 0;
            fdtab[i].flags = 0;
            return i;
        }
    }
    return -1;
}

long sys_read(int fd, void *buf, unsigned long count)
{
    if (fd < 3 || fd >= FD_MAX || !fdtab[fd].used)
        return -1;
    node_t *n = fdtab[fd].node;
    if (!n || n->type != NODE_FILE)
        return -1;
    size_t ofs = fdtab[fd].ofs;
    if (ofs >= n->size)
        return 0;
    size_t remain = n->size - ofs;
    if (count > remain)
        count = remain;
    int r = fs_read(n, (char *)buf, count);
    fdtab[fd].ofs += r;
    return r;
}
long sys_write(int fd, const void *buf, unsigned long count)
{
    const char *c = (const char *)buf;
    if (fd == 1 || fd == 2)
    {
        for (unsigned long i = 0; i < count; i++)
            kputc(c[i]);
        return (long)count;
    }
    if (fd < 3 || fd >= FD_MAX || !fdtab[fd].used)
        return -1;
    node_t *n = fdtab[fd].node;
    if (!n || n->type != NODE_FILE)
        return -1;
    // For now writes always append; adjust offset to end
    if (fs_write(n, c, count, 1) == 0)
    {
        fdtab[fd].ofs = n->size;
        return (long)count;
    }
    return -1;
}
long sys_open(const char *path, int flags, int mode)
{
    (void)mode;
    node_t *cwd = fs_cwd();
    node_t *f = fs_lookup(cwd, path);
    if (!f)
    {
        if (flags & 1)
        { // O_CREAT simplified
            f = fs_create_file(cwd, path);
        }
    }
    if (!f)
        return -1;
    int fd = alloc_fd(f);
    return fd;
}
long sys_close(int fd)
{
    if (fd < 3 || fd >= FD_MAX || !fdtab[fd].used)
        return -1;
    fdtab[fd].used = 0;
    fdtab[fd].node = 0;
    fdtab[fd].ofs = 0;
    return 0;
}
long sys_fork(void) { return -1; }
long sys_execve(const char *path, char *const argv[], char *const envp[])
{
    (void)path;
    (void)argv;
    (void)envp;
    return -1;
}

static void fill_stat(node_t *n, struct stat *st)
{
    if (!st)
        return;
    st->st_dev = 0;
    st->st_ino = (uintptr_t)n;
    st->st_mode = (n->type == NODE_DIR ? S_IFDIR : S_IFREG) | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_size = n->size;
    st->st_blksize = 512;
    st->st_blocks = (n->size + 511) / 512;
    st->st_atime = st->st_mtime = st->st_ctime = 0;
}

long sys_stat(const char *path, void *ubuf)
{
    if (!path || !ubuf)
        return -1;
    node_t *cwd = fs_cwd();
    node_t *n = fs_lookup(cwd, path);
    if (!n)
        return -1;
    fill_stat(n, (struct stat *)ubuf);
    return 0;
}
long sys_fstat(int fd, void *ubuf)
{
    if (fd < 3 || fd >= FD_MAX || !fdtab[fd].used || !ubuf)
        return -1;
    node_t *n = fdtab[fd].node;
    if (!n)
        return -1;
    fill_stat(n, (struct stat *)ubuf);
    return 0;
}
long sys_lseek(int fd, long off, int whence)
{
    if (fd < 3 || fd >= FD_MAX || !fdtab[fd].used)
        return -1;
    node_t *n = fdtab[fd].node;
    size_t newofs;
    switch (whence)
    {
    case 0:
        newofs = (off < 0) ? 0 : (size_t)off;
        break; // SEEK_SET
    case 1:
        newofs = fdtab[fd].ofs + off;
        break; // SEEK_CUR
    case 2:
        newofs = n->size + off;
        break; // SEEK_END
    default:
        return -1;
    }
    if (newofs > n->size)
        newofs = n->size; // clamp
    fdtab[fd].ofs = newofs;
    return (long)newofs;
}

long syscall_dispatch(long num, long a1, long a2, long a3, long a4, long a5, long a6)
{
    (void)a5;
    (void)a6;
    switch (num)
    {
    case SYS_read:
        return sys_read((int)a1, (void *)a2, (unsigned long)a3);
    case SYS_write:
        return sys_write((int)a1, (const void *)a2, (unsigned long)a3);
    case SYS_open:
        return sys_open((const char *)a1, (int)a2, (int)a3);
    case SYS_close:
        return sys_close((int)a1);
    case SYS_stat:
        return sys_stat((const char *)a1, (void *)a2);
    case SYS_fstat:
        return sys_fstat((int)a1, (void *)a2);
    case SYS_lseek:
        return sys_lseek((int)a1, a2, (int)a3);
    case SYS_fork:
        return sys_fork();
    case SYS_execve:
        return sys_execve((const char *)a1, (char *const *)a2, (char *const *)a3);
    default:
        return -1;
    }
}
