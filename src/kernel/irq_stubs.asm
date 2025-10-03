BITS 64
SECTION .text

%macro MAKE_IRQ 1
global irq_stub_%1
irq_stub_%1:
    push qword %1        ; push IRQ number
    jmp irq_stub_common
%endmacro

%assign i 0
%rep 16
MAKE_IRQ i
%assign i i+1
%endrep

global irq_stub_table_label
irq_stub_table_label:
%assign j 0
%rep 16
    dq irq_stub_%+j
%assign j j+1
%endrep

extern irq_common_dispatch
irq_stub_common:
    ; Stack: [irq_number]
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    mov rdi, [rsp + 11*8] ; irq number (after pushes)
    call irq_common_dispatch
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    add rsp, 8 ; drop irq number
    iretq

global irq_stub_ptrs
irq_stub_ptrs:
%assign k 0
%rep 16
    dq irq_stub_%+k
%assign k k+1
%endrep
