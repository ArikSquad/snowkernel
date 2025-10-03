MBALIGN equ 1<<0
MEMINFO equ 1<<1
FLAGS   equ MBALIGN | MEMINFO
MAGIC   equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot progbits alloc noexec nowrite align=4
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
bits 32
global _start
_start:
    mov esp, stack_top
    ; Zero .bss
    extern __bss_start
    extern __bss_end
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    shr ecx, 2  ; divide by 4 for dwords
    xor eax, eax
    rep stosd
    extern kernel_main
    call kernel_main
.hang:
    hlt
    jmp .hang
