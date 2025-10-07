#include "elf.h"
#include "kprint.h"

#include "pmm.h"
#include "vm.h"

int elf_inspect(const void *image, size_t size,
                elf_seg_info_t *segs, int max_segs,
                uintptr_t *entry_out) {
    if (size < sizeof(Elf64_Ehdr)) return -1;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr*)image;
    if (*(const uint32_t*)eh->e_ident != ELF_MAGIC) return -1;
    if (eh->e_phoff + (uint64_t)eh->e_phnum * eh->e_phentsize > size) return -1;
    const unsigned char *base = (const unsigned char*)image;
    int count = 0;
    for (uint16_t i=0;i<eh->e_phnum;i++) {
        const Elf64_Phdr *ph = (const Elf64_Phdr*)(base + eh->e_phoff + (uint64_t)i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD) continue;
        if (ph->p_offset + ph->p_filesz > size) return -1;
        if (count < max_segs) {
            segs[count].vaddr = ph->p_vaddr;
            segs[count].memsz = ph->p_memsz;
            segs[count].filesz = ph->p_filesz;
            segs[count].offset = ph->p_offset;
            segs[count].flags = ph->p_flags;
        }
        count++;
    }
    if (entry_out) *entry_out = eh->e_entry;
    return count;
}

int elf_load(const void *image, size_t size, uintptr_t *entry, uintptr_t *out_base, uintptr_t *out_end)
{
    if (size < sizeof(Elf64_Ehdr))
        return -1;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)image;
    uint32_t magic = *(const uint32_t *)eh->e_ident;
    if (magic != ELF_MAGIC)
    {
        kprintf("[elf] bad magic %x\n", magic);
        return -1;
    }
    if (eh->e_phoff + (uint64_t)eh->e_phnum * eh->e_phentsize > size)
        return -1;
    const unsigned char *base = (const unsigned char *)image;
    uintptr_t min_addr = (uintptr_t)-1;
    uintptr_t max_addr = 0;
    for (uint16_t i = 0; i < eh->e_phnum; i++)
    {
        const Elf64_Phdr *ph = (const Elf64_Phdr *)(base + eh->e_phoff + (uint64_t)i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD)
            continue;
        if (ph->p_offset + ph->p_filesz > size)
            return -1;
        uintptr_t vaddr = ph->p_vaddr;
        uintptr_t memsz = ph->p_memsz;
        uintptr_t filesz = ph->p_filesz;
        uintptr_t num_pages = (memsz + 4095) / 4096;
        uintptr_t flags = PTE_PRESENT | PTE_USER;
        if (ph->p_flags & PF_W)
            flags |= PTE_WRITABLE;
        for (uintptr_t p = 0; p < num_pages; p++)
        {
            uintptr_t phys = (uintptr_t)pmm_alloc_page();
            if (!phys)
            {
                kprintf("[elf] pmm_alloc_page failed ph=%u page=%u/%u\n", (unsigned)i, (unsigned)p, (unsigned)num_pages);
                return -1;
            }
            vm_map_page(vaddr + p * 4096, phys, flags);
        }
        // Copy data
        unsigned char *dst = (unsigned char *)vaddr;
        const unsigned char *src = base + ph->p_offset;
        for (uint64_t b = 0; b < filesz; b++)
            dst[b] = src[b];
        // Zero bss
        for (uint64_t b = filesz; b < memsz; b++)
            dst[b] = 0;
        if (vaddr < min_addr)
            min_addr = vaddr;
        if (vaddr + memsz > max_addr)
            max_addr = vaddr + memsz;
    }
    if (min_addr == (uintptr_t)-1)
        return -1;
    if (entry)
        *entry = (uintptr_t)eh->e_entry; // not relocated yet
    if (out_base)
        *out_base = min_addr;
    if (out_end)
        *out_end = max_addr;
    kprintf("[elf] loaded segments base=%x end=%x entry=%x\n", (unsigned)min_addr, (unsigned)max_addr, (unsigned)eh->e_entry);
    return 0;
}
