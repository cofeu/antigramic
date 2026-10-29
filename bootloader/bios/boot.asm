bits 16
org 0x7C00

section .text
global _start

_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    ; BIOS passes the boot device in DL. Preserve it before using DX for UART I/O.
    mov byte [boot_drive], dl

    mov dx, 0x3F8
    mov al, 'B'
    out dx, al

    ; Serial init
    mov dx, 0x3F8 + 1
    mov al, 0x00
    out dx, al
    mov dx, 0x3F8 + 3
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8 + 0
    mov al, 0x0C
    out dx, al
    mov dx, 0x3F8 + 1
    mov al, 0x00
    out dx, al
    mov dx, 0x3F8 + 3
    mov al, 0x03
    out dx, al
    mov dx, 0x3F8 + 2
    mov al, 0xC7
    out dx, al
    mov dx, 0x3F8 + 4
    mov al, 0x0B
    out dx, al

    mov dx, 0x3F8
    mov al, 'D'
    out dx, al

    ; Load stage2 to 0x80000, clear of the kernel staging area and page tables.
    mov word [dap_count], 8
    mov word [dap_off], 0x0000
    mov word [dap_seg], 0x8000
    mov dword [dap_lba_low], 1
    mov dword [dap_lba_high], 0
    call read_sectors_lba
    jc .err

    mov al, '3'
    out dx, al

    ; Stage the kernel at 0x40000; stage2 relocates it to its linked address.
    mov al, 'K'
    out dx, al
    mov word [dap_count], 64
    mov word [dap_off], 0x0000
    mov word [dap_seg], 0x4000
    mov dword [dap_lba_low], 129
    mov dword [dap_lba_high], 0
    mov cx, 4
.load_kernel_chunk:
    call read_sectors_lba
    jc .err2
    add dword [dap_lba_low], 64
    add word [dap_seg], 0x0800
    loop .load_kernel_chunk

    mov al, 'K'
    out dx, al
    mov al, 'J'
    out dx, al

    mov ax, 0x8000
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov ax, 0x7000
    mov ss, ax
    mov sp, 0xF000
    jmp 0x8000:0x0000

.err:
    mov al, 'E'
    out dx, al
    jmp $

.err2:
    mov al, 'R'
    out dx, al
    jmp $

read_sectors_lba:
    pushad
    mov di, 0x10
.retry:
    pushad
    mov ah, 0x42
    mov dl, byte [boot_drive]
    mov si, dap
    int 0x13
    popad
    jnc .done
    dec di
    jnz .retry
    stc
.done:
    popad
    ret

align 16
dap:
    db 0x10
    db 0
dap_count:
    dw 0
dap_off:
    dw 0
dap_seg:
    dw 0
dap_lba_low:
    dd 0
dap_lba_high:
    dd 0

boot_drive:
    db 0

times 510-($-$$) db 0
dw 0xAA55
