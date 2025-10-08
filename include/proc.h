#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../src/fs/fs.h"

typedef struct fd_entry {
    int used;
    node_t *node;
    size_t ofs;
    int flags;
} fd_entry_t;

#define PROC_FD_MAX 32
#define PROC_MAX_PAGES 256

/* Process states */
#define PROC_STATE_UNUSED   0
#define PROC_STATE_RUNNING  1
#define PROC_STATE_ZOMBIE   2

typedef struct process {
    int pid;
    struct process *parent;
    int state;
    fd_entry_t fds[PROC_FD_MAX];
    uint64_t pml4_phys;
    uint64_t alloc_pages[PROC_MAX_PAGES];
    int alloc_count;
    uint64_t kernel_saved_rsp;
    uint64_t kernel_saved_rip;
    uint64_t brk_start;  // base of heap region (virtual user addr)
    uint64_t brk_curr;   // current program break
    uint64_t image_hi;   // highest mapped byte (+1) of current image
} process_t;

/* Process management */
process_t *proc_current(void);
void proc_init(void);
process_t *proc_find_by_pid(int pid);
process_t *proc_alloc(void);
void proc_free(process_t *p);
void proc_set_current(process_t *p);

/* File descriptor management */
int proc_alloc_fd(node_t *n);
fd_entry_t *proc_get_fd(int fd);

/* Memory management */
int proc_set_pml4(uint64_t phys);
uint64_t proc_get_pml4(void);
int proc_add_allocated_page(uint64_t phys);
void proc_free_allocated_pages(void);
uint64_t proc_get_brk(void);
int proc_set_brk(uint64_t newbrk);

