#include "proc.h"
#include "../kernel/kprint.h"

static process_t init_proc;
static process_t *current = 0;
static int next_pid = 1;

process_t *proc_current(void) { return current; }

void proc_init(void)
{
    for (int i = 0; i < PROC_FD_MAX; i++)
    {
        init_proc.fds[i].used = 0;
        init_proc.fds[i].node = 0;
        init_proc.fds[i].ofs = 0;
        init_proc.fds[i].flags = 0;
    }
    init_proc.pid = next_pid++;
    init_proc.parent = 0;
    init_proc.state = 0;
    current = &init_proc;
    init_proc.pml4_phys = 0;
    init_proc.alloc_count = 0;
    for (int i = 0; i < PROC_MAX_PAGES; i++)
        init_proc.alloc_pages[i] = 0;
    init_proc.kernel_saved_rsp = 0;
    init_proc.kernel_saved_rip = 0;
    init_proc.brk_start = 0;
    init_proc.brk_curr = 0;
    init_proc.image_hi = 0;
    for (int i = 0; i < 3; i++)
        current->fds[i].used = 1;
    kprintf("[proc] init pid=%d created\n", init_proc.pid);
}

int proc_set_pml4(uint64_t phys)
{
    if (!current)
        return -1;
    current->pml4_phys = phys;
    return 0;
}

uint64_t proc_get_pml4(void)
{
    if (!current)
        return 0;
    return current->pml4_phys;
}

int proc_add_allocated_page(uint64_t phys)
{
    if (!current)
        return -1;
    if (current->alloc_count >= PROC_MAX_PAGES)
        return -1;
    current->alloc_pages[current->alloc_count++] = phys;
    return 0;
}

void proc_free_allocated_pages(void)
{
    if (!current)
        return;
    current->alloc_count = 0;
}

int proc_alloc_fd(node_t *n)
{
    if (!current)
        return -1;
    for (int i = 3; i < PROC_FD_MAX; i++)
    {
        if (!current->fds[i].used)
        {
            current->fds[i].used = 1;
            current->fds[i].node = n;
            current->fds[i].ofs = 0;
            current->fds[i].flags = 0;
            return i;
        }
    }
    return -1;
}

fd_entry_t *proc_get_fd(int fd)
{
    if (!current || fd < 0 || fd >= PROC_FD_MAX)
        return 0;
    if (!current->fds[fd].used)
        return 0;
    return &current->fds[fd];
}

uint64_t proc_get_brk(void)
{
    if (!current)
        return 0;
    return current->brk_curr;
}

int proc_set_brk(uint64_t newbrk)
{
    if (!current)
        return -1;
    current->brk_curr = newbrk;
    return 0;
}
