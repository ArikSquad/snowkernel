.global _start
.extern main
.extern _exit
_start:
    /* argc/argv not yet constructed; pass 0 */
    xor %rdi,%rdi   /* argc=0 */
    xor %rsi,%rsi   /* argv=NULL */
    call main
    /* return value in %eax -> rdi */
    mov %eax,%edi
    call _exit
    hlt
