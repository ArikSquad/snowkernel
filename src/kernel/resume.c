#include <stdint.h>

uint64_t kernel_resume_rsp = 0;
uint64_t kernel_resume_rip = 0;

void kernel_resume(void)
{
    if (!kernel_resume_rip) return;
    __asm__ volatile (
        "mov %0, %%rsp\n"
        "jmp *%1\n"
        :
        : "r" (kernel_resume_rsp), "r" (kernel_resume_rip)
    );
}
