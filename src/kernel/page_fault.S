BITS 64
GLOBAL page_fault_isr
EXTERN page_fault_handler_c
page_fault_isr:
    push qword 0
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
    push r12
    push r13
    push r14
    push r15
    mov rdi, rsp
    call page_fault_handler_c
    add rsp, 8*15+8
    add rsp, 8
    iretq