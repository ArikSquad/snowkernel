BITS 32
SECTION .text

%macro MAKE_IRQ 1
global irq32_stub_%1
irq32_stub_%1:
    push dword %1
    jmp irq32_stub_common
%endmacro

%assign i 0
%rep 16
MAKE_IRQ i
%assign i i+1
%endrep

global irq32_stub_ptrs
irq32_stub_ptrs:
%assign j 0
%rep 16
    dd irq32_stub_%+j
%assign j j+1
%endrep

extern irq_common_dispatch
irq32_stub_common:
    pusha
    mov eax, [esp + 32] ; irq number (after pusha pushes 8 regs)
    push eax            ; arg
    call irq_common_dispatch
    add esp, 4
    popa
    add esp, 4 ; discard irq number
    iretd
