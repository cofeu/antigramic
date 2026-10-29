section .text
global _start
extern init_process

_start:
    mov rsp, 0x7FFFFFFF0000
    call init_process
    mov rax, 2
    syscall
    hlt
    jmp $