section .text
global _start
extern kmain

_start:
    cli
    mov esp, 0x90000

    lgdt [gdt_ptr]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    mov eax, 0x100000
    mov cr3, eax
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov dword [0x100000], 0x101003
    mov edi, 0x101000
    mov ecx, 1024
    mov eax, 0x102003
.fill_pde:
    mov dword [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .fill_pde

    mov eax, cr4
    or eax, 1 << 4
    mov cr4, eax

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp 0x08:protected_mode

bits 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    push dword 0
    push dword 0x200000
    call kmain

    cli
    hlt
    jmp $

gdt_start:
    dd 0, 0
gdt_code:
    dw 0xFFFF
    dw 0
    db 0
    db 0x9A
    db 0xCF
    db 0
gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 0x92
    db 0xCF
    db 0
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start