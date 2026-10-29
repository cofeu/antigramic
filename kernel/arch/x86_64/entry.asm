section .text
global _start
extern kmain
extern __bss_start
extern __bss_end

_start:
    cli
    ; serial init
    mov dx, 0x3F8+1
    mov al, 0x00
    out dx, al
    mov dx, 0x3F8+3
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8+0
    mov al, 0x03
    out dx, al
    mov dx, 0x3F8+1
    mov al, 0x00
    out dx, al
    mov dx, 0x3F8+3
    mov al, 0x03
    out dx, al
    mov dx, 0x3F8+2
    mov al, 0xC7
    out dx, al
    mov dx, 0x3F8+4
    mov al, 0x0B
    out dx, al

    mov dx, 0x3F8
    mov al, 'E'
    out dx, al

    ; clear BSS
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    xor rax, rax
    rep stosb

    mov al, 'N'
    out dx, al

    ; call C entry
    call kmain

    mov al, 'X'
    out dx, al

.hang:
    hlt
    jmp .hang
