global int80_entry
extern syscall_thunk
section .text
bits 32
int80_entry:
    pusha
    xor ebp, ebp
    mov eax, [esp+28] ; saved eax
    mov ebx, [esp+36] ; saved ebx
    mov ecx, [esp+24] ; saved ecx
    mov edx, [esp+32] ; saved edx
    mov esi, [esp+20] ; saved esi
    mov edi, [esp+16] ; saved edi
    push 0       ; a6
    push 0       ; a5
    push 0       ; a4
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax
    call syscall_thunk
    add esp, 9*4
    mov [esp+28], eax ; store return into saved eax slot
    popa
    iretd
