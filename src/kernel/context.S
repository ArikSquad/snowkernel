    .text
    .global context_restore
    .type context_restore, @function
context_restore:
    # void context_restore(uint64_t rsp, uint64_t rip)
    movq %rdi, %rsp
    jmp *%rsi
    .size context_restore, .-context_restore
