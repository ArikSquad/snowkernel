#pragma once
#include <stddef.h>
#include <stdint.h>

#define ELF_MAGIC 0x464C457FU

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct
{
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7

#define PT_GNU_EH_FRAME 0x6474e550
#define PT_GNU_STACK    0x6474e551
#define PT_GNU_RELRO    0x6474e552

int elf_load(const void *image, size_t size, uintptr_t *entry, uintptr_t *base, uintptr_t *end);

typedef struct {
    uintptr_t vaddr;
    uintptr_t memsz;
    uintptr_t filesz;
    uintptr_t offset;
    uint32_t flags; /* PF_* */
} elf_seg_info_t;

#define ELF_SEG_INFO_DEFINED 1
int elf_inspect(const void *image, size_t size,
                elf_seg_info_t *segs, int max_segs,
                uintptr_t *entry_out);
