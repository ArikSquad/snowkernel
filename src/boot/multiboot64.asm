MBALIGN equ 1<<0
MEMINFO equ 1<<1
FLAGS   equ MBALIGN | MEMINFO
MAGIC   equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom: resb 32768
stack_top:

section .text
bits 32
global _start
extern kernel_main64
_start:
    mov esp, stack_top
    ; zero .bss
    extern __bss_start
    extern __bss_end
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    shr ecx, 2
    xor eax, eax
    rep stosd
    lgdt [gdt_descriptor]
    ; Enable PAE
    mov eax, cr4
    or eax, 1<<5
    mov cr4, eax
    ; setup identity PML4 -> PDPT -> PD -> 2MB page (simplified, assumes low memory)
    mov eax, pml4
    mov cr3, eax
    ; Enable long mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1<<8
    wrmsr
    ; paging + protection
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
    jmp 0x08:long_mode_entry

align 8
gdt:
    dq 0
    dq 0x00AF9A000000FFFF ; kernel code
    dq 0x00AF92000000FFFF ; kernel data
    ; User segments: same as kernel but DPL=3 (access bytes 0xFA code, 0xF2 data), base=0
    dq 0x00AFFA000000FFFF ; user code (DPL=3)
    dq 0x00AFF2000000FFFF ; user data (DPL=3)
    ; TSS descriptor uses two GDT entries (128-bit). We'll place an empty
    ; placeholder here; it will be filled with the address/limit below.
    dq 0
    dq 0
gdt_descriptor:
    dw gdt_descriptor - gdt - 1
    dd gdt

align 4096
; Identity map first 128MB using 2MB pages (64 entries) to cover large kernel + embedded binaries
; Each entry: base | present(1) | rw(2) | ps(1<<7)
; 0x83 = 0b1000_0011 (present|rw|ps)
align 4096
pml4: dq pdpt + 0x003          ; present|rw
align 4096
pdpt: dq pd + 0x003, 0,0,0,0,0,0,0
align 4096
pd:
%assign i 0
%rep 64 ; 64 * 2MB = 128MB
    dq (i*0x200000) | 0x83
%assign i i+1
%endrep

bits 64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, stack_top
    ; -----------------------------------------------------------------
    ; Build and load a minimal TSS descriptor (index 5 in GDT) so later
    ; privilege transitions (when user mode is added) have a valid RSP0.
    ; Incorrect descriptor construction can cause a #GP on LTR leading
    ; to an early triple fault (observed hang). Re-implement descriptor
    ; creation following the AMD64 spec precisely.
    ; -----------------------------------------------------------------
    lea rax, [tss]
    mov qword [tss + 0], rsp        ; tss.rsp0 = current kernel stack
    ; Descriptor layout (64-bit TSS available type=0x9):
    ;  Byte 0-1: limit (size-1)
    ;  Byte 2-3: base 15:0
    ;  Byte 4  : base 23:16
    ;  Byte 5  : type(4)=0x9 | 0 | DPL=0 | P=1 => 10001001b = 0x89
    ;  Byte 6  : limit 19:16 (0) | AVL=0 | L=0 | D/B=0 | G=0
    ;  Byte 7  : base 31:24
    ;  Bytes 8-15: base 63:32 then reserved
    mov rbx, rax               ; RBX = base
    mov rcx, 0x67              ; limit = sizeof(tss)-1 (104 bytes -> 0x68, -1 => 0x67)
    ; Write lower 16 bits of limit
    mov word [gdt + 5*8 + 0], cx
    ; base 15:0
    mov dx, bx
    mov word [gdt + 5*8 + 2], dx
    ; base 23:16
    shr rbx, 16
    mov byte [gdt + 5*8 + 4], bl
    ; type / flags (present | available 64-bit TSS)
    mov byte [gdt + 5*8 + 5], 0x89
    ; limit 19:16 and flags (all zero for now)
    mov byte [gdt + 5*8 + 6], 0x00
    ; base 31:24
    shr rbx, 8
    mov byte [gdt + 5*8 + 7], bl
    ; Upper 8 bytes: base 63:32
    mov rdx, rax
    shr rdx, 32
    mov dword [gdt + 5*8 + 8], edx
    mov dword [gdt + 5*8 + 12], 0
    ; Finally load TR selector (index 5 * 8 = 0x28)
    mov ax, 0x28
    ltr ax
    ; Simple early debug over port 0xE9 (QEMU debug console) to prove we reached long mode
    mov dx, 0xE9
    mov rsi, early_msg
.early_out_loop:
    lodsb
    test al, al
    jz .after_early_out
    out dx, al
    jmp .early_out_loop
.after_early_out:
    call kernel_main64
.halt:
    hlt
    jmp .halt

; 64-bit Task State Segment for hardware stack switching on ring change.
align 16
tss:
    ; Struct layout: reserve 104 bytes (enough for rsp0..2 and ISTs and I/O map)
    ; We'll zero initialize it; RSP0 will be set by the kernel if needed.
    times 104 db 0

early_msg: db "[boot] Long mode + TSS OK",0
