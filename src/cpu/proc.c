#include "proc.h"
#include "../kernel/kprint.h"
#include "../kernel/string.h"

#define MAX_PROCESSES 64

process_t process_table[MAX_PROCESSES];  /* Make it accessible for syscalls */
static process_t *current = 0;
static int next_pid = 1;

process_t *proc_current(void) { return current; }

void proc_init(void)
{
    /* Initialize process table */
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        process_table[i].pid = 0;
        process_table[i].state = PROC_STATE_UNUSED;
        process_table[i].parent = 0;
        process_table[i].pml4_phys = 0;
        process_table[i].alloc_count = 0;
        process_table[i].kernel_saved_rsp = 0;
        process_table[i].kernel_saved_rip = 0;
        process_table[i].brk_start = 0;
        process_table[i].brk_curr = 0;
        process_table[i].image_hi = 0;
        process_table[i].umask = 0022;
        process_table[i].uid = 0;
        process_table[i].gid = 0;
        for (int j = 0; j < PROC_FD_MAX; j++)
        {
            process_table[i].fds[j].used = 0;
            process_table[i].fds[j].node = 0;
            process_table[i].fds[j].ofs = 0;
            process_table[i].fds[j].flags = 0;
        }
    }
    
    /* Create init process (PID 1) */
    process_t *init = &process_table[0];
    init->pid = next_pid++;
    init->parent = 0;
    init->state = PROC_STATE_RUNNING;
    init->pml4_phys = 0;
    init->alloc_count = 0;
    init->kernel_saved_rsp = 0;
    init->kernel_saved_rip = 0;
    init->brk_start = 0;
    init->brk_curr = 0;
    init->image_hi = 0;
    init->umask = 0022;
    init->uid = 0;
    init->gid = 0;
    
    /* Mark stdin/stdout/stderr as used */
    for (int i = 0; i < 3; i++)
        init->fds[i].used = 1;
    
    current = init;
    kprintf("[proc] init pid=%d created (kernel process)\n", init->pid);
}

process_t *proc_find_by_pid(int pid)
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].state != PROC_STATE_UNUSED && process_table[i].pid == pid)
            return &process_table[i];
    }
    return 0;
}

process_t *proc_alloc(void)
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].state == PROC_STATE_UNUSED)
        {
            process_t *p = &process_table[i];
            kmemset(p, 0, sizeof(process_t));
            p->pid = next_pid++;
            p->state = PROC_STATE_RUNNING;
            p->parent = current;
            kprintf("[proc] allocated new process pid=%d\n", p->pid);
            return p;
        }
    }
    kprintf("[proc] ERROR: process table full\n");
    return 0;
}

void proc_free(process_t *p)
{
    if (!p) return;
    kprintf("[proc] freeing process pid=%d\n", p->pid);
    p->state = PROC_STATE_UNUSED;
    p->pid = 0;
}

void proc_set_current(process_t *p)
{
    current = p;
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
