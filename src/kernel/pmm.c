#include "pmm.h"
#include "kprint.h"

extern char __bss_end; // linker symbol

#define PAGE_SIZE 4096UL
#define PMM_LIMIT (128UL * 1024 * 1024)

static uintptr_t pmm_cur = 0;
static uintptr_t pmm_end = 0;
static void *pmm_free_list = NULL;

void pmm_init(void)
{
    if (pmm_cur) return;
    uintptr_t start = ((uintptr_t)&__bss_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    pmm_cur = start;
    pmm_end = PMM_LIMIT;
    kprintf("[pmm] start=%x limit=%x\n", (unsigned)start, (unsigned)pmm_end);
}

void *pmm_alloc_page(void)
{
    if (!pmm_cur)
        pmm_init();
    if (pmm_free_list) {
        void *ret = pmm_free_list;
        pmm_free_list = *(void**)pmm_free_list;
        unsigned char *p = (unsigned char *)ret;
        for (size_t i = 0; i < PAGE_SIZE; i++) p[i] = 0;
        return ret;
    }
    if (pmm_cur + PAGE_SIZE > pmm_end)
        return 0;
    void *ret = (void *)pmm_cur;
    unsigned char *p = (unsigned char *)ret;
    for (size_t i = 0; i < PAGE_SIZE; i++) p[i] = 0;
    pmm_cur += PAGE_SIZE;
    return ret;
}

void pmm_free_page(void *p)
{
    if (!p) return;
    *(void**)p = pmm_free_list;
    pmm_free_list = p;
}
