#include "vm.h"
#include "pmm.h"
#include "kprint.h"
#include <stdint.h>

#define PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)
#define PDP_INDEX(virt) (((virt) >> 30) & 0x1FF)
#define PD_INDEX(virt) (((virt) >> 21) & 0x1FF)
#define PT_INDEX(virt) (((virt) >> 12) & 0x1FF)

typedef uint64_t pte_t;

static inline uint64_t get_cr3(void)
{
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline void set_cr3(uint64_t cr3)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

uint64_t vm_get_cr3(void)
{
    return get_cr3();
}

void vm_set_cr3(uint64_t cr3)
{
    set_cr3(cr3);
}

uint64_t vm_clone_current_pml4(void)
{
    uint64_t newp = (uint64_t)pmm_alloc_page();
    if (!newp)
        return 0;
    uint64_t cur = vm_get_cr3() & ~0xFFFULL;
    uint64_t *src = (uint64_t *)cur;
    uint64_t *dst = (uint64_t *)newp;
    for (int i = 0; i < 512; i++)
        dst[i] = src[i];
    return newp;
}

void vm_clear_kernel_mappings(uint64_t pml4_phys)
{
    pte_t *pml4 = (pte_t *)pml4_phys;
    for (int i = 0; i < 512; i++)
    {
        pml4[i] = 0;
    }
}

static inline void invlpg(uint64_t addr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(addr));
}

void vm_init(void)
{
    kprintf("[vm] initialized\n");
}

static pte_t *alloc_table(void)
{
    uint64_t phys = (uint64_t)pmm_alloc_page();
    if (!phys)
        return NULL;

    uint64_t *table = (uint64_t *)phys;
    for (int i = 0; i < 512; i++)
    {
        table[i] = 0;
    }
    return (pte_t *)phys;
}

void vm_map_page(uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t pml4_phys = get_cr3() & ~0xFFF;
    pte_t *pml4 = (pte_t *)pml4_phys;

    uint64_t pml4_idx = PML4_INDEX(virt);
    if (!(pml4[pml4_idx] & PTE_PRESENT))
    {
        pte_t *pdp = alloc_table();
        if (!pdp)
            return;
        pml4[pml4_idx] = (uint64_t)pdp | PTE_PRESENT | PTE_WRITABLE;
    }

    pte_t *pdp = (pte_t *)(pml4[pml4_idx] & ~0xFFF);
    uint64_t pdp_idx = PDP_INDEX(virt);
    if (!(pdp[pdp_idx] & PTE_PRESENT))
    {
        pte_t *pd = alloc_table();
        if (!pd)
            return;
        pdp[pdp_idx] = (uint64_t)pd | PTE_PRESENT | PTE_WRITABLE;
    }

    pte_t *pd = (pte_t *)(pdp[pdp_idx] & ~0xFFF);
    uint64_t pd_idx = PD_INDEX(virt);
    if (!(pd[pd_idx] & PTE_PRESENT))
    {
        pte_t *pt = alloc_table();
        if (!pt)
            return;
        pd[pd_idx] = (uint64_t)pt | PTE_PRESENT | PTE_WRITABLE;
    }

    pte_t *pt = (pte_t *)(pd[pd_idx] & ~0xFFF);
    uint64_t pt_idx = PT_INDEX(virt);
    pt[pt_idx] = (phys & ~0xFFF) | flags;

    invlpg(virt);
}

void vm_map_page_pml4(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags)
{
    pte_t *pml4 = (pte_t *)pml4_phys;

    uint64_t pml4_idx = PML4_INDEX(virt);
    if (!(pml4[pml4_idx] & PTE_PRESENT))
    {
        pte_t *pdp = alloc_table();
        if (!pdp)
            return;
        pml4[pml4_idx] = (uint64_t)pdp | PTE_PRESENT | PTE_WRITABLE;
    }

    pte_t *pdp = (pte_t *)(pml4[pml4_idx] & ~0xFFF);
    uint64_t pdp_idx = PDP_INDEX(virt);
    if (!(pdp[pdp_idx] & PTE_PRESENT))
    {
        pte_t *pd = alloc_table();
        if (!pd)
            return;
        pdp[pdp_idx] = (uint64_t)pd | PTE_PRESENT | PTE_WRITABLE;
    }

    pte_t *pd = (pte_t *)(pdp[pdp_idx] & ~0xFFF);
    uint64_t pd_idx = PD_INDEX(virt);
    if (!(pd[pd_idx] & PTE_PRESENT))
    {
        pte_t *pt = alloc_table();
        if (!pt)
            return;
        pd[pd_idx] = (uint64_t)pt | PTE_PRESENT | PTE_WRITABLE;
    }

    pte_t *pt = (pte_t *)(pd[pd_idx] & ~0xFFF);
    uint64_t pt_idx = PT_INDEX(virt);
    pt[pt_idx] = (phys & ~0xFFF) | flags;
}

static uint64_t kernel_cr3 = 0;

void vm_set_kernel_cr3(uint64_t cr3)
{
    kernel_cr3 = cr3 & ~0xFFFULL;
}

uint64_t vm_get_kernel_cr3(void)
{
    return kernel_cr3;
}

void vm_unmap_page(uint64_t virt)
{
    uint64_t pml4_phys = get_cr3() & ~0xFFF;
    pte_t *pml4 = (pte_t *)pml4_phys;

    uint64_t pml4_idx = PML4_INDEX(virt);
    if (!(pml4[pml4_idx] & PTE_PRESENT))
        return;

    pte_t *pdp = (pte_t *)(pml4[pml4_idx] & ~0xFFF);
    uint64_t pdp_idx = PDP_INDEX(virt);
    if (!(pdp[pdp_idx] & PTE_PRESENT))
        return;

    pte_t *pd = (pte_t *)(pdp[pdp_idx] & ~0xFFF);
    uint64_t pd_idx = PD_INDEX(virt);
    if (!(pd[pd_idx] & PTE_PRESENT))
        return;

    pte_t *pt = (pte_t *)(pd[pd_idx] & ~0xFFF);
    uint64_t pt_idx = PT_INDEX(virt);
    pt[pt_idx] = 0;

    invlpg(virt);
}

uint64_t vm_get_phys(uint64_t virt)
{
    uint64_t pml4_phys = get_cr3() & ~0xFFF;
    pte_t *pml4 = (pte_t *)pml4_phys;

    uint64_t pml4_idx = PML4_INDEX(virt);
    if (!(pml4[pml4_idx] & PTE_PRESENT))
        return 0;

    pte_t *pdp = (pte_t *)(pml4[pml4_idx] & ~0xFFF);
    uint64_t pdp_idx = PDP_INDEX(virt);
    if (!(pdp[pdp_idx] & PTE_PRESENT))
        return 0;

    pte_t *pd = (pte_t *)(pdp[pdp_idx] & ~0xFFF);
    uint64_t pd_idx = PD_INDEX(virt);
    if (!(pd[pd_idx] & PTE_PRESENT))
        return 0;

    pte_t *pt = (pte_t *)(pd[pd_idx] & ~0xFFF);
    uint64_t pt_idx = PT_INDEX(virt);
    if (!(pt[pt_idx] & PTE_PRESENT))
        return 0;

    return (pt[pt_idx] & ~0xFFF) | (virt & 0xFFF);
}

uint64_t vm_get_phys_pml4(uint64_t pml4_phys, uint64_t virt)
{
    pte_t *pml4 = (pte_t *)pml4_phys;

    uint64_t pml4_idx = PML4_INDEX(virt);
    if (!(pml4[pml4_idx] & PTE_PRESENT))
        return 0;

    pte_t *pdp = (pte_t *)(pml4[pml4_idx] & ~0xFFF);
    uint64_t pdp_idx = PDP_INDEX(virt);
    if (!(pdp[pdp_idx] & PTE_PRESENT))
        return 0;

    pte_t *pd = (pte_t *)(pdp[pdp_idx] & ~0xFFF);
    uint64_t pd_idx = PD_INDEX(virt);
    if (!(pd[pd_idx] & PTE_PRESENT))
        return 0;

    pte_t *pt = (pte_t *)(pd[pd_idx] & ~0xFFF);
    uint64_t pt_idx = PT_INDEX(virt);
    if (!(pt[pt_idx] & PTE_PRESENT))
        return 0;

    return (pt[pt_idx] & ~0xFFF) | (virt & 0xFFF);
}
