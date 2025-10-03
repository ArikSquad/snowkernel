global _start
extern kernel_main
section .text
bits 32
_start:
    call kernel_main
.hang: hlt
    jmp .hang
