MBALIGN equ 1<<0
MEMINFO equ 1<<1
FLAGS   equ MBALIGN | MEMINFO
MAGIC   equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom: resb 32768
stack_top:

section .text
bits 32
global _start
extern kernel_main64
_start:
    mov esp, stack_top
    ; zero .bss
    extern __bss_start
    extern __bss_end
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    shr ecx, 2
    xor eax, eax
    rep stosd
    lgdt [gdt_descriptor]
    ; Enable PAE
    mov eax, cr4
    or eax, 1<<5
    mov cr4, eax
    ; setup identity PML4 -> PDPT -> PD -> 2MB page (simplified, assumes low memory)
    mov eax, pml4
    mov cr3, eax
    ; Enable long mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1<<8
    wrmsr
    ; paging + protection
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
    jmp 0x08:long_mode_entry

align 8
gdt:
    dq 0
    dq 0x00AF9A000000FFFF ; code
    dq 0x00AF92000000FFFF ; data
gdt_descriptor:
    dw gdt_descriptor - gdt - 1
    dd gdt

align 4096
; first 8MB using 2MB pages (4 entries)
align 4096
; present + rw bits (0x3) by adding since NASM '|' on relocations is invalid
pml4: dq pdpt + 0x003
align 4096
pdpt: dq pd + 0x003, 0,0,0,0,0,0,0
align 4096
pd:
    dq 0x0000000000000083 ; 0-2MB present|rw|2MB
    dq 0x0000000000200083 ; 2-4MB
    dq 0x0000000000400083 ; 4-6MB
    dq 0x0000000000600083 ; 6-8MB

bits 64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, stack_top
    call kernel_main64
.halt:
    hlt
    jmp .halt
