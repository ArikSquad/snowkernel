#include "syscall.h"
#include "kprint.h"
#include "../fs/fs.h"
#include "string.h"
#include "stat.h" /* defines S_IFDIR/S_IFREG/S_IFCHR */
#include "elf.h" /* ensure elf_seg_info_t visible before usage */
#include "proc.h"
#include "pmm.h"
#include "vm.h"
#include "tty.h"
#include <stdint.h>
#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif

// ensure ig
#ifndef ELF_SEG_INFO_DEFINED
typedef struct {
    uintptr_t vaddr;
    uintptr_t memsz;
    uintptr_t filesz;
    uintptr_t offset;
    uint32_t flags;
} elf_seg_info_t;
#endif

#define MAX_FDS PROC_FD_MAX

long sys_read(int fd, void *buf, unsigned long count)
{
    fd_entry_t *e = proc_get_fd(fd);
    if (fd < 3 || !e)
        return -1;
    node_t *n = e->node;
    if (!n)
        return -1;
    if (n->type == NODE_CHAR) {
        extern int tty_read(struct tty*, char*, size_t);
        struct tty *t = (struct tty*)n->data;
        if (!t) return -1;
        int r = tty_read(t, (char*)buf, count);
        return r;
    }
    if (n->type != NODE_FILE)
        return -1;
    size_t ofs = e->ofs;
    if (ofs >= n->size)
        return 0;
    size_t remain = n->size - ofs;
    if (count > remain)
        count = remain;
    int r = fs_read(n, (char *)buf, count);
    e->ofs += r;
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
    fd_entry_t *e = proc_get_fd(fd);
    if (fd < 3 || !e)
        return -1;
    node_t *n = e->node;
    if (!n)
        return -1;
    if (n->type == NODE_CHAR) {
        extern int tty_write(struct tty*, const char*, size_t);
        struct tty *t = (struct tty*)n->data;
        if (!t) return -1;
        return tty_write(t, c, count);
    }
    if (n->type != NODE_FILE)
        return -1;
    if (fs_write(n, c, count, 1) == 0)
    {
        e->ofs = n->size;
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
    int fd = proc_alloc_fd(f);
    return fd;
}
long sys_close(int fd)
{
    fd_entry_t *e = proc_get_fd(fd);
    if (fd < 3 || !e)
        return -1;
    e->used = 0;
    e->node = 0;
    e->ofs = 0;
    return 0;
}
long sys_dup(int oldfd)
{
    process_t *proc = proc_current();
    if (oldfd < 0 || oldfd >= MAX_FDS || !proc->fds[oldfd].node)
        return -1;
    int newfd = proc_alloc_fd(proc->fds[oldfd].node);
    if (newfd < 0)
        return -1;
    proc->fds[newfd] = proc->fds[oldfd];
    return newfd;
}

long sys_dup2(int oldfd, int newfd)
{
    process_t *proc = proc_current();
    if (oldfd < 0 || oldfd >= MAX_FDS || !proc->fds[oldfd].node)
        return -1;
    if (newfd < 0 || newfd >= MAX_FDS)
        return -1;
    if (proc->fds[newfd].node)
        sys_close(newfd);
    proc->fds[newfd] = proc->fds[oldfd];
    return newfd;
}

long sys_pipe(int *pipefd)
{
    // TODO: implement pipes
    (void)pipefd;
    return -1; // not implemented
}

long sys_fork(void)
{
    return -1; // not implemented
}
long sys_execve(const char *path, char *const argv[], char *const envp[])
{
    (void)envp;
    if (!path || !*path)
        return -1;
    node_t *cwd = fs_cwd();
    node_t *n = fs_lookup(cwd, path);
    if (!n || n->type != NODE_FILE)
        return -1;
    kprintf("[execve] start path='%s' size=%u\n", path, (unsigned)n->size);

    /* Inspect ELF without mapping into kernel address space */
    elf_seg_info_t segs[16];
    uintptr_t entry = 0; 
    int seg_count = elf_inspect(n->data, n->size, segs, 16, &entry);
    if (seg_count < 0) {
        kprintf("[execve] not a valid ELF (%s)\n", path);
        return -1;
    }
    kprintf("[execve] ELF entry=%x segs=%d\n", (unsigned)entry, seg_count);
    if (seg_count >= 16) {
        kprintf("[execve] too many segments (>%d) unsupported\n", 16);
        return -1;
    }
    /* Basic feature rejection: look for PT_INTERP or dynamic / unsupported flags */
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr*)n->data;
    const unsigned char *baseptr_img = (const unsigned char*)n->data;
    /* Sanity checks on program header table before iterating */
    if (ehdr->e_phentsize < sizeof(Elf64_Phdr)) {
        kprintf("[execve] e_phentsize=%u < sizeof(Phdr) -> reject\n", (unsigned)ehdr->e_phentsize);
        return -1;
    }
    uint64_t ph_table_bytes = (uint64_t)ehdr->e_phentsize * ehdr->e_phnum;
    if (ehdr->e_phoff > n->size || ph_table_bytes > n->size || ehdr->e_phoff + ph_table_bytes > n->size) {
        kprintf("[execve] ph table out of range: off=%x size=%x total=%x img=%x -> reject\n",
                (unsigned)ehdr->e_phoff, (unsigned)ph_table_bytes, (unsigned)(ehdr->e_phoff + ph_table_bytes), (unsigned)n->size);
        return -1;
    }
    kprintf("[execve] phoff=%x phentsz=%u phnum=%u (table_bytes=%x)\n", (unsigned)ehdr->e_phoff, (unsigned)ehdr->e_phentsize, (unsigned)ehdr->e_phnum, (unsigned)ph_table_bytes);
    int saw_interp = 0, saw_dynamic = 0; 
    for (uint16_t i=0;i<ehdr->e_phnum;i++) {
        const uint64_t ph_off = ehdr->e_phoff + (uint64_t)i * ehdr->e_phentsize;
        const Elf64_Phdr *ph = (const Elf64_Phdr*)(baseptr_img + ph_off);
        if (ph_off + sizeof(Elf64_Phdr) > n->size) {
            kprintf("[execve] ph[%u] truncated (off=%x) -> stop\n", (unsigned)i, (unsigned)ph_off);
            break;
        }
        kprintf("[execve] ph[%u] type=%x off=%x vaddr=%x filesz=%x memsz=%x flags=%x align=%x\n",
                (unsigned)i, (unsigned)ph->p_type, (unsigned)ph->p_offset, (unsigned)ph->p_vaddr,
                (unsigned)ph->p_filesz, (unsigned)ph->p_memsz, (unsigned)ph->p_flags, (unsigned)ph->p_align);
        if (ph->p_type == PT_INTERP) { saw_interp = 1; }
        else if (ph->p_type == PT_DYNAMIC) { saw_dynamic = 1; }
    }
    if (saw_interp) {
        kprintf("[execve] PT_INTERP present -> dynamic binary unsupported (reject)\n");
        return -1;
    }
    if (saw_dynamic) {
        kprintf("[execve] PT_DYNAMIC present -> dynamic/reloc binary unsupported (reject)\n");
        return -1;
    }
    /* Light scan for 64-bit SYSCALL instruction (0x0F 0x05). If present, warn: we only implement int 0x80 path. */
    int found_syscall = 0;
    const unsigned char *scan = (const unsigned char*)n->data;
    for (size_t i=0;i+1<n->size && i<4096; i++) { /* limit scan for speed */
        if (scan[i]==0x0F && scan[i+1]==0x05) { found_syscall=1; break; }
    }
    if (found_syscall) {
        kprintf("[execve] WARNING: binary uses SYSCALL instruction (unsupported) -> abort\n");
        return -1;
    }

    /* Reset brk tracking for this process (fresh image). */
    process_t *pc = proc_current();
    if (pc) { pc->brk_start = 0; pc->brk_curr = 0; }

    uint64_t new_pml4 = vm_clone_current_pml4();
    if (!new_pml4)
    {
        kprintf("[execve] vm clone failed\n");
        return -1;
    }

    /* Preserve kernel mappings so syscalls/IRQs can function after user entry. */

    /* Map & load segments explicitly using inspected metadata */
    for (int si=0; si<seg_count; si++) {
        uintptr_t vaddr_aligned = segs[si].vaddr & ~0xFFFULL;
        uintptr_t delta = segs[si].vaddr - vaddr_aligned;
        uintptr_t filesz = segs[si].filesz + delta; /* include leading delta in first page copy logic */
        uintptr_t memsz = segs[si].memsz + delta;
        uintptr_t num_pages = (memsz + 4095)/4096;
        uint64_t flags = PTE_PRESENT | PTE_USER | PTE_WRITABLE;
        if (!(segs[si].flags & PF_W)) flags &= ~PTE_WRITABLE;
        kprintf("[execve] seg %d vaddr=%x mem=%x file=%x pages=%u flags=%x\n",
                si, (unsigned)segs[si].vaddr, (unsigned)segs[si].memsz, (unsigned)segs[si].filesz,
                (unsigned)num_pages, (unsigned)flags);
        for (uintptr_t p=0; p<num_pages; p++) {
            void *phys = pmm_alloc_page();
            if (!phys) { kprintf("[execve] pmm alloc failed seg=%d page=%u\n", si, (unsigned)p); return -1; }
            uintptr_t page_offset = p*4096;
            for (int z=0; z<4096; z++) ((char*)phys)[z]=0; /* pre-zero */
            /* copy portion overlapping file */
            if (page_offset < filesz) {
                uintptr_t copy = filesz - page_offset; if (copy>4096) copy=4096;
                kmemcpy((char*)phys, (char*)n->data + segs[si].offset + page_offset - delta, copy);
            }
            vm_map_page_pml4(new_pml4, vaddr_aligned + p*4096, (uint64_t)phys, flags);
            proc_add_allocated_page((uint64_t)phys);
        }
        /* Track high watermark for heap placement */
        uintptr_t seg_end = segs[si].vaddr + segs[si].memsz;
        if (pc && seg_end > pc->image_hi) pc->image_hi = seg_end;
    }

    const uintptr_t USER_STACK_TOP = 0x7FFFFFF0000ULL;
    void *ustack_phys = pmm_alloc_page();
    if (!ustack_phys)
    {
        kprintf("[execve] stack alloc failed\n");
        return -1;
    }
    vm_map_page_pml4(new_pml4, USER_STACK_TOP - 4096, (uint64_t)ustack_phys, PTE_PRESENT | PTE_USER | PTE_WRITABLE);
    proc_add_allocated_page((uint64_t)ustack_phys);

    proc_set_pml4(new_pml4);
    proc_add_allocated_page(new_pml4);

    uint64_t cur_rsp_k;
    __asm__ volatile("mov %%rsp, %0" : "=r"(cur_rsp_k));
    uint64_t kstack_page = cur_rsp_k & ~0xFFFULL;
    vm_map_page_pml4(new_pml4, kstack_page, kstack_page, PTE_PRESENT | PTE_WRITABLE);

    uint64_t cur_rsp;
    __asm__ volatile("mov %%rsp, %0" : "=r"(cur_rsp));
    process_t *curp = proc_current();
    if (curp)
    {
        curp->kernel_saved_rsp = cur_rsp;
        curp->kernel_saved_rip = (uint64_t)&&after_enter;
    }

after_enter:;
    kprintf("[execve] prepared user image %s entry=%x pml4=%x\n", path, (unsigned)entry, (unsigned)new_pml4);
    uint64_t user_sp = USER_STACK_TOP;
    uint64_t str_off = 0x100;
    char *phys_stack = (char *)ustack_phys;
    size_t alen = 0;
    while (path[alen])
        alen++;
    if (alen + 1 > 4096 - str_off)
        return -1;

    for (size_t i = 0; i <= alen; i++)
        phys_stack[str_off + i] = path[i];
    uint64_t arg0_vaddr = (USER_STACK_TOP - 4096) + str_off;
    user_sp -= 8; /* argv NULL terminator */
    uint64_t argv0_ptr_slot = user_sp - 8;
    user_sp = argv0_ptr_slot;
    uint64_t ptr_off = (uint64_t)(argv0_ptr_slot - (USER_STACK_TOP - 4096));
    if (ptr_off + 8 > 4096)
        return -1;
    *(uint64_t *)((char *)ustack_phys + ptr_off) = arg0_vaddr;
    uint64_t null_off = (uint64_t)(user_sp + 8 - (USER_STACK_TOP - 4096));
    if (null_off + 8 <= 4096)
        *(uint64_t *)((char *)ustack_phys + null_off) = 0;
    user_sp &= ~0xFULL;
    kprintf("[execve] entering user entry=%x sp=%x pml4=%x (final)\n", (unsigned)entry, (unsigned)user_sp, (unsigned)new_pml4);
    enter_user(entry, user_sp, new_pml4);
    kprintf("[execve] ERROR: returned from enter_user (aborting exec)\n");
    return -1;
}

static void fill_stat(node_t *n, struct stat *st)
{
    if (!st)
        return;
    st->st_dev = 0;
    st->st_ino = (uintptr_t)n;
    if (n->type == NODE_DIR)
        st->st_mode = S_IFDIR;
    else if (n->type == NODE_CHAR)
        st->st_mode = S_IFCHR;
    else
        st->st_mode = S_IFREG;
    st->st_mode |= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
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
    fd_entry_t *e = proc_get_fd(fd);
    if (fd < 3 || !e || !ubuf)
        return -1;
    node_t *n = e->node;
    if (!n)
        return -1;
    fill_stat(n, (struct stat *)ubuf);
    return 0;
}

int sys_isatty(int fd) {
    fd_entry_t *e = proc_get_fd(fd);
    if (!e) return 0;
    if (e->node && e->node->type == NODE_CHAR) return 1;
    return 0;
}
long sys_lseek(int fd, long off, int whence)
{
    fd_entry_t *e = proc_get_fd(fd);
    if (fd < 3 || !e)
        return -1;
    node_t *n = e->node;
    size_t newofs;
    switch (whence)
    {
    case 0:
        newofs = (off < 0) ? 0 : (size_t)off;
        break; // SEEK_SET
    case 1:
        newofs = e->ofs + off;
        break; // SEEK_CUR
    case 2:
        newofs = n->size + off;
        break; // SEEK_END
    default:
        return -1;
    }
    if (newofs > n->size)
        newofs = n->size; // clamp
    e->ofs = newofs;
    return (long)newofs;
}

long sys_exit(int code)
{
    kprintf("[proc] exit code=%d\n", code);
    process_t *cur = proc_current();
    if (cur)
    {
        if (cur->pml4_phys)
        {
            for (int i = 0; i < cur->alloc_count; i++)
            {
                pmm_free_page((void *)cur->alloc_pages[i]);
            }
            proc_free_allocated_pages();
            uint64_t kernel_cr3 = vm_get_kernel_cr3();
            vm_set_cr3(kernel_cr3);
            cur->pml4_phys = 0;
        }
    }

    extern void context_restore(uint64_t rsp, uint64_t rip);
    if (cur && cur->kernel_saved_rip)
    {
        context_restore(cur->kernel_saved_rsp, cur->kernel_saved_rip);
    }
    return 0;
}

long sys_brk(void *addr)
{
    process_t *p = proc_current();
    if (!p) return -1;
    if (p->brk_start == 0) {
        uint64_t base;
        if (p->image_hi) {
            /* heap after image_hi + 1MB guard */
            base = (p->image_hi + 0x00100000ULL + 0xFFFULL) & ~0xFFFULL;
        } else {
            /* default if image_hi unknown */
            base = (0x01000000ULL + 0x00100000ULL + 0xFFFULL) & ~0xFFFULL; /* 16MB + 1MB guard */
        }
        p->brk_start = base;
        p->brk_curr = base;
    }
    if (addr == 0) {
        return (long)p->brk_curr;
    }
    uint64_t new_brk = (uint64_t)addr;
    if (new_brk < p->brk_start) new_brk = p->brk_start;
    /* allocate pages for any newly covered region */
    uint64_t old = p->brk_curr;
    if (new_brk > old) {
        uint64_t start_page = (old + 0xFFFULL) & ~0xFFFULL;
        uint64_t end_page = (new_brk + 0xFFFULL) & ~0xFFFULL;
        for (uint64_t va = start_page; va < end_page; va += 0x1000ULL) {
            void *phys = pmm_alloc_page();
            if (!phys) break; /* out of memory -> stop early */
            vm_map_page_pml4(proc_get_pml4(), va, (uint64_t)phys, PTE_PRESENT | PTE_USER | PTE_WRITABLE);
            proc_add_allocated_page((uint64_t)phys);
        }
    }
    p->brk_curr = new_brk;
    return (long)p->brk_curr;
}

/* Process identification syscalls */
long sys_getpid(void)
{
    process_t *p = proc_current();
    return p ? p->pid : 1;
}

long sys_getppid(void)
{
    process_t *p = proc_current();
    if (!p || !p->parent) return 0;
    return p->parent->pid;
}

/* User/Group ID syscalls - simplified (always return 0 for root) */
long sys_getuid(void) { return 0; }
long sys_getgid(void) { return 0; }
long sys_geteuid(void) { return 0; }
long sys_getegid(void) { return 0; }

/* Directory operation syscalls */
long sys_chdir(const char *path)
{
    if (!path) return -1;
    node_t *cwd = fs_cwd();
    node_t *n = fs_lookup(cwd, path);
    if (!n || n->type != NODE_DIR) return -1;
    fs_set_cwd(n);
    return 0;
}

long sys_getcwd(char *buf, unsigned long size)
{
    if (!buf || size == 0) return -1;
    node_t *cwd = fs_cwd();
    if (!cwd) {
        if (size >= 2) {
            buf[0] = '/';
            buf[1] = '\0';
        }
        return (long)buf;
    }
    
    /* Build path backwards from cwd to root */
    char tmp[256];
    int pos = 255;
    tmp[pos] = '\0';
    
    node_t *n = cwd;
    while (n && n->parent) {
        int len = 0;
        while (n->name[len]) len++;
        if (pos - len - 1 < 0) return -1; /* path too long */
        pos -= len;
        for (int i = 0; i < len; i++) tmp[pos + i] = n->name[i];
        pos--;
        tmp[pos] = '/';
        n = n->parent;
    }
    
    if (pos == 255) { /* root directory */
        if (size >= 2) {
            buf[0] = '/';
            buf[1] = '\0';
        }
        return (long)buf;
    }
    
    /* Copy result to user buffer */
    unsigned long len = 255 - pos;
    if (len + 1 > size) return -1; /* buffer too small */
    for (unsigned long i = 0; i <= len; i++) {
        buf[i] = tmp[pos + i];
    }
    return (long)buf;
}

long sys_mkdir(const char *path, int mode)
{
    (void)mode; /* ignore mode for now */
    if (!path) return -1;
    node_t *cwd = fs_cwd();
    
    /* Check if already exists */
    node_t *existing = fs_lookup(cwd, path);
    if (existing) return -1; /* already exists */
    
    /* Simple path handling - extract parent and name */
    const char *last_slash = path;
    const char *p = path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }
    
    node_t *parent = cwd;
    const char *name = path;
    
    if (last_slash != path) {
        /* Need to navigate to parent directory */
        char parent_path[256];
        int len = last_slash - path;
        if (len >= 256) return -1;
        for (int i = 0; i < len; i++) parent_path[i] = path[i];
        parent_path[len] = '\0';
        parent = fs_lookup(cwd, parent_path);
        if (!parent || parent->type != NODE_DIR) return -1;
        name = last_slash + 1;
    } else if (*path == '/') {
        parent = fs_root();
        name = path + 1;
    }
    
    node_t *new_dir = fs_mkdir(parent, name);
    return new_dir ? 0 : -1;
}

long sys_rmdir(const char *path)
{
    if (!path) return -1;
    node_t *cwd = fs_cwd();
    node_t *n = fs_lookup(cwd, path);
    if (!n || n->type != NODE_DIR) return -1;
    if (n->child) return -1; /* directory not empty */
    
    /* Extract parent and name */
    const char *last_slash = path;
    const char *p = path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }
    
    node_t *parent = n->parent ? n->parent : cwd;
    const char *name = (last_slash != path) ? last_slash + 1 : path;
    
    return fs_unlink(parent, name) ? 0 : -1;
}

long sys_unlink(const char *path)
{
    if (!path) return -1;
    node_t *cwd = fs_cwd();
    node_t *n = fs_lookup(cwd, path);
    if (!n || n->type == NODE_DIR) return -1; /* can't unlink directories */
    
    /* Extract parent and name */
    const char *last_slash = path;
    const char *p = path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }
    
    node_t *parent = n->parent ? n->parent : cwd;
    const char *name = (last_slash != path) ? last_slash + 1 : path;
    
    return fs_unlink(parent, name) ? 0 : -1;
}

long sys_rename(const char *oldpath, const char *newpath)
{
    if (!oldpath || !newpath) return -1;
    node_t *cwd = fs_cwd();
    node_t *old = fs_lookup(cwd, oldpath);
    if (!old) return -1;
    
    /* Extract parent and names */
    const char *new_last_slash = newpath;
    const char *p = newpath;
    while (*p) {
        if (*p == '/') new_last_slash = p;
        p++;
    }
    
    const char *old_last_slash = oldpath;
    p = oldpath;
    while (*p) {
        if (*p == '/') old_last_slash = p;
        p++;
    }
    
    node_t *old_parent = old->parent ? old->parent : cwd;
    const char *old_name = (old_last_slash != oldpath) ? old_last_slash + 1 : oldpath;
    const char *new_name = (new_last_slash != newpath) ? new_last_slash + 1 : newpath;
    
    /* Use fs_rename if same parent, otherwise more complex */
    return fs_rename(old_parent, old_name, new_name);
}

long sys_access(const char *path, int mode)
{
    (void)mode; /* simplified - just check if file exists */
    if (!path) return -1;
    node_t *cwd = fs_cwd();
    node_t *n = fs_lookup(cwd, path);
    return n ? 0 : -1;
}

/* Directory reading - simplified implementation */
long sys_readdir(int fd, void *dirp, unsigned int count)
{
    (void)fd;
    (void)dirp;
    (void)count;
    return -1; /* not yet implemented */
}

/* File control operations */
long sys_fcntl(int fd, int cmd, long arg)
{
    fd_entry_t *e = proc_get_fd(fd);
    if (!e) return -1;
    
    /* Simplified - handle basic commands */
    switch (cmd) {
        case 0: /* F_DUPFD */
            return sys_dup(fd);
        case 1: /* F_GETFD */
            return 0; /* no close-on-exec flag set */
        case 2: /* F_SETFD */
            (void)arg;
            return 0;
        case 3: /* F_GETFL */
            return e->flags;
        case 4: /* F_SETFL */
            e->flags = (int)arg;
            return 0;
        default:
            return -1;
    }
}

long sys_ioctl(int fd, unsigned long request, unsigned long arg)
{
    (void)fd;
    (void)request;
    (void)arg;
    return -1; /* simplified - not implemented */
}

long sys_umask(int mask)
{
    process_t *p = proc_current();
    (void)p;
    (void)mask;
    return 0022; /* default umask */
}

/* Time syscalls - simplified */
long sys_time(long *tloc)
{
    long t = 0; /* no real time tracking yet */
    if (tloc) *tloc = t;
    return t;
}

long sys_gettimeofday(void *tv, void *tz)
{
    (void)tz;
    if (!tv) return -1;
    struct {
        long tv_sec;
        long tv_usec;
    } *timeval = tv;
    timeval->tv_sec = 0;
    timeval->tv_usec = 0;
    return 0;
}

/* Process control */
long sys_waitpid(int pid, int *status, int options)
{
    (void)pid;
    (void)options;
    if (status) *status = 0;
    return -1; /* no child processes yet */
}

long syscall_dispatch(long num, long a1, long a2, long a3, long a4, long a5, long a6)
{
    (void)a4;
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
    case SYS_pipe:
        return sys_pipe((int *)a1);
    case SYS_dup:
        return sys_dup((int)a1);
    case SYS_dup2:
        return sys_dup2((int)a1, (int)a2);
    case SYS_fork:
        return sys_fork();
    case SYS_execve:
        return sys_execve((const char *)a1, (char *const *)a2, (char *const *)a3);
    case SYS_exit:
        return sys_exit((int)a1);
    case SYS_brk:
        return sys_brk((void*)a1);
    case SYS_getpid:
        return sys_getpid();
    case SYS_getppid:
        return sys_getppid();
    case SYS_getuid:
        return sys_getuid();
    case SYS_getgid:
        return sys_getgid();
    case SYS_geteuid:
        return sys_geteuid();
    case SYS_getegid:
        return sys_getegid();
    case SYS_chdir:
        return sys_chdir((const char *)a1);
    case SYS_getcwd:
        return sys_getcwd((char *)a1, (unsigned long)a2);
    case SYS_mkdir:
        return sys_mkdir((const char *)a1, (int)a2);
    case SYS_rmdir:
        return sys_rmdir((const char *)a1);
    case SYS_unlink:
        return sys_unlink((const char *)a1);
    case SYS_rename:
        return sys_rename((const char *)a1, (const char *)a2);
    case SYS_access:
        return sys_access((const char *)a1, (int)a2);
    case SYS_readdir:
        return sys_readdir((int)a1, (void *)a2, (unsigned int)a3);
    case SYS_fcntl:
        return sys_fcntl((int)a1, (int)a2, a3);
    case SYS_ioctl:
        return sys_ioctl((int)a1, (unsigned long)a2, (unsigned long)a3);
    case SYS_umask:
        return sys_umask((int)a1);
    case SYS_time:
        return sys_time((long *)a1);
    case SYS_gettimeofday:
        return sys_gettimeofday((void *)a1, (void *)a2);
    case SYS_waitpid:
        return sys_waitpid((int)a1, (int *)a2, (int)a3);
    default:
        return -1;
    }
}
