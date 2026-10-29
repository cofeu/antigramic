section .text
global context_switch

context_switch:
    mov [rdi + 24], rsp
    mov [rdi + 32], rbp
    mov [rdi + 40], rbx
    mov [rdi + 48], r12
    mov [rdi + 56], r13
    mov [rdi + 64], r14
    mov [rdi + 72], r15

    mov rsp, [rsi + 24]
    mov rbp, [rsi + 32]
    mov rbx, [rsi + 40]
    mov r12, [rsi + 48]
    mov r13, [rsi + 56]
    mov r14, [rsi + 64]
    mov r15, [rsi + 72]

    ret