BITS 64
SECTION .text
global int80_entry64
extern syscall_thunk

int80_entry64:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; Linux int80 uses: eax=num, ebx, ecx, edx, esi, edi, ebp for args historically.
    ; We standardize on: RAX=num, RBX=a1, RCX=a2, RDX=a3, RSI=a4, RDI=a5, R8=a6 for int80 path to map easily.
    ; But our userland not defined yet; we will mirror 32-bit order for simplicity.
    ; Retrieve saved values from stack (top after pushes is original RAX at [rsp])
    mov rax, [rsp]
    mov rbx, [rsp+8*13] ; saved rbx position? Simpler: we already saved in order, rebuild manually
    ; Instead of complex extraction, just use saved registers directly by reloading before call.
    ; We'll assume arguments were placed like 32-bit: RAX=num, RBX=a1, RCX=a2, RDX=a3, RSI=a4, RDI=a5, R8=a6.

    ; Push a6..a1 then num for syscall_thunk(num,a1..a6)
    ; Switch to kernel page table before handling syscall
    extern vm_get_kernel_cr3
    mov rcx, [rel vm_get_kernel_cr3]
    ; vm_get_kernel_cr3 is a function; call it to get kernel cr3 into rax
    call vm_get_kernel_cr3
    mov rcx, rax
    mov cr3, rcx
    push r8      ; a6
    push rdi     ; a5
    push rsi     ; a4
    push rdx     ; a3
    push rcx     ; a2
    push rbx     ; a1
    push rax     ; num
    call syscall_thunk
    add rsp, 7*8

    ; Return value now in RAX. Restore registers (reverse order of pushes excluding original RAX which we popped logically)
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    iretq
