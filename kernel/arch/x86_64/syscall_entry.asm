section .text
global syscall_entry

syscall_entry:
    swapgs
    mov [gs:0], rsp
    mov rsp, [gs:8]

    push r11
    push rcx
    push rax
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9

    mov rdi, rsp
    extern syscall_handler
    call syscall_handler

    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    pop rax
    pop rcx
    pop r11

    mov [gs:8], rsp
    mov rsp, [gs:0]
    swapgs
    sysretq