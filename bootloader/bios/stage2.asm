bits 16
org 0x0000

start:
    cli
    mov ax, 0x8000
    mov ds, ax
    mov es, ax
    mov ax, 0x7000
    mov ss, ax
    mov sp, 0xF000
    sti

    mov dx, 0x3F8
    mov al, 'S'
    out dx, al

    in al, 0x92
    or al, 2
    out 0x92, al
    mov al, 'A'
    out dx, al

    ; Configure the QEMU/Bochs VBE adapter for a portrait 544x960 XRGB display.
    mov dx, 0x01CE
    xor ax, ax
    out dx, ax
    mov dx, 0x01CF
    in ax, dx
    cmp ax, 0xB0C4
    jb .video_error
    mov dx, 0x01CE
    mov ax, 4
    out dx, ax
    mov dx, 0x01CF
    xor ax, ax
    out dx, ax
    mov dx, 0x01CE
    mov ax, 1
    out dx, ax
    mov dx, 0x01CF
    mov ax, 544
    out dx, ax
    mov dx, 0x01CE
    mov ax, 2
    out dx, ax
    mov dx, 0x01CF
    mov ax, 960
    out dx, ax
    mov dx, 0x01CE
    mov ax, 3
    out dx, ax
    mov dx, 0x01CF
    mov ax, 32
    out dx, ax
    mov dx, 0x01CE
    mov ax, 6
    out dx, ax
    mov dx, 0x01CF
    mov ax, 544
    out dx, ax
    mov dx, 0x01CE
    mov ax, 4
    out dx, ax
    mov dx, 0x01CF
    mov ax, 0x41
    out dx, ax
    jmp .video_ready
.video_error:
    mov dx, 0x3F8
    mov al, 'V'
    out dx, al
.halt_video:
    cli
    hlt
    jmp .halt_video
.video_ready:
    mov dx, 0x3F8

    ; DS already points at the loaded stage2 segment (0x8000).
    lgdt [gdt32_ptr - start]
    mov al, 'G'
    out dx, al

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    db 0x66, 0xea
    dd 0x80000 + pmode
    dw 0x08

bits 32
pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x1F00000

    ; Relocate the BIOS-loaded flat kernel to its linked address at 0x8000.
    mov esi, 0x40000
    mov edi, 0x8000
    ; Relocate the complete 128 KiB kernel load window.
    mov ecx, 32768
    cld
    rep movsd

    ; Build identity maps for the first 4 GiB, including the VBE framebuffer.
    ; Keep boot page tables beyond the kernel's static UI and transition buffers.
    mov edi, 0x2000000
    xor eax, eax
    mov ecx, 0x1800
    rep stosd

    mov dword [0x2000000], 0x2001003
    mov dword [0x2000004], 0
    mov dword [0x2001000], 0x2002003
    mov dword [0x2001004], 0
    mov dword [0x2001008], 0x2003003
    mov dword [0x200100C], 0
    mov dword [0x2001010], 0x2004003
    mov dword [0x2001014], 0
    mov dword [0x2001018], 0x2005003
    mov dword [0x200101C], 0

    mov edi, 0x2002000
    mov eax, 0x83
    mov ecx, 2048
.map_2m:
    mov [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000
    add edi, 8
    loop .map_2m

    ; Enable PAE, long mode, and paging.
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    mov eax, 0x2000000
    mov cr3, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov eax, cr0
    or eax, (1 << 31) | 1
    mov cr0, eax

    jmp 0x18:(0x80000 + long_mode - start)

bits 64
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x1F00000
    xor rbp, rbp
    mov rax, 0x8000
    jmp rax

bits 32

gdt32_start:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00AF9A000000FFFF
gdt32_end:

gdt32_ptr:
    dw gdt32_end - gdt32_start - 1
    dd 0x80000 + gdt32_start
