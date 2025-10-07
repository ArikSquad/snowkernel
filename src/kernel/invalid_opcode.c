#include "kprint.h"
#include "proc.h"
#include "pmm.h"
#include "vm.h"
#include "syscall.h"

void invalid_opcode_handler_c(void *frame) {
    kprintf("[trap] #UD invalid opcode encountered (likely unsupported instruction)\n");
    /* The assembly stub pushes rax,rcx,rdx,rbx,rbp,rsi,rdi,r8,r9,r10,r11 (11 regs)
       and the CPU pushed RIP/CS/RFLAGS before transfering control to the ISR.
       Therefore the saved RIP is located at ((uint64_t*)frame)[11]. */
    uint64_t *stk = (uint64_t*)frame;
    uintptr_t found_rip = 0;
    if (stk) {
        kprintf("[trap] stack dump (first 32 qwords):\n");
        for (unsigned i = 0; i < 32; i++) {
            kprintf("  %u: %p\n", i, (void*)stk[i]);
            /* look for a value in kernel text region (0x00101000-0x00120000) */
            uintptr_t v = (uintptr_t)stk[i];
            if (!found_rip && v >= 0x00101000 && v < 0x00200000) {
                found_rip = v;
            }
        }
    } else {
        kprintf("[trap] no stack frame provided\n");
    }
    if (found_rip) {
        kprintf("[trap] heuristic RIP=%p\n", (void*)found_rip);
    } else {
        kprintf("[trap] heuristic RIP not found in stack snapshot\n");
    }

    process_t *cur = proc_current();
    if (cur && cur->pml4_phys) {
        kprintf("[trap] terminating process due to #UD (pid=%u)\n", (unsigned)(cur->pid));
        sys_exit(-1);
        return;
    }
    kprintf("[trap] in kernel context -> continuing\n");
    return;
}
