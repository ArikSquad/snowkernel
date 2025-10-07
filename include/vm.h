#ifndef VM_H
#define VM_H

#include <stdint.h>

#define PAGE_SIZE 4096

#define PTE_PRESENT (1 << 0)
#define PTE_WRITABLE (1 << 1)
#define PTE_USER (1 << 2)
#define PTE_PWT (1 << 3)
#define PTE_PCD (1 << 4)
#define PTE_ACCESSED (1 << 5)
#define PTE_DIRTY (1 << 6)
#define PTE_PS (1 << 7)
#define PTE_GLOBAL (1 << 8)

void vm_init(void);
void vm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);

void vm_unmap_page(uint64_t virt);

uint64_t vm_get_phys(uint64_t virt);
uint64_t vm_clone_current_pml4(void);
uint64_t vm_get_cr3(void);
void vm_set_cr3(uint64_t cr3);
void enter_user(uint64_t entry, uint64_t user_rsp, uint64_t user_cr3);
void vm_clear_kernel_mappings(uint64_t pml4_phys);
void vm_map_page_pml4(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags);
void vm_set_kernel_cr3(uint64_t cr3);
uint64_t vm_get_kernel_cr3(void);
uint64_t vm_get_phys_pml4(uint64_t pml4_phys, uint64_t virt);

#endif