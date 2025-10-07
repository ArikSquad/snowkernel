BITS 64
SECTION .text
global enter_user

enter_user:
    mov     rcx, rdx
    mov     cr3, rcx

    ; Segment selectors for user mode
    mov     ax, 0x23            ; user data selector (index 4, RPL=3)
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     r8, 0x1B            ; user code selector (index 3, RPL=3)

    ; Build an iretq frame on current kernel stack to transition
    ; Stack layout (top->bottom as pushed):
    ;   RIP (entry)
    ;   CS  (user code selector | RPL=3)
    ;   RFLAGS (with IF=1)
    ;   RSP (user stack pointer)
    ;   SS  (user data selector | RPL=3)

    push    qword 0x23          ; SS already includes RPL=3
    push    rsi                 ; user stack pointer
    pushfq
    pop     rax
    or      eax, 0x200          ; ensure IF=1
    push    rax
    push    qword 0x1B          ; CS already includes RPL=3
    push    rdi                 ; RIP = entry point

    xor     rax, rax
    xor     rbx, rbx
    xor     rcx, rcx
    xor     rdx, rdx
    xor     rsi, rsi
    xor     rdi, rdi
    xor     r8, r8
    xor     r9, r9
    xor     r10, r10
    xor     r11, r11
    xor     r12, r12
    xor     r13, r13
    xor     r14, r14
    xor     r15, r15

    iretq
