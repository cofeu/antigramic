section .text
global context_switch

; void context_switch(struct task *prev, struct task *next)
; RDI = prev, RSI = next
context_switch:
    ; Save prev task context
    mov [rdi + 0x00], rsp
    mov [rdi + 0x08], rbp
    mov [rdi + 0x10], rbx
    mov [rdi + 0x18], r12
    mov [rdi + 0x20], r13
    mov [rdi + 0x28], r14
    mov [rdi + 0x30], r15
    call .save_rip
.save_rip:
    pop rax
    mov [rdi + 0x38], rax
    pushfq
    pop rax
    mov [rdi + 0x40], rax
    
    ; Switch page table if needed
    mov rax, [rsi + 0x48]  ; pml4
    test rax, rax
    jz .skip_cr3
    mov cr3, rax
.skip_cr3:
    
    ; Load next task context
    mov rsp, [rsi + 0x00]
    mov rbp, [rsi + 0x08]
    mov rbx, [rsi + 0x10]
    mov r12, [rsi + 0x18]
    mov r13, [rsi + 0x20]
    mov r14, [rsi + 0x28]
    mov r15, [rsi + 0x30]
    mov rax, [rsi + 0x38]   ; rip
    push rax
    mov rax, [rsi + 0x40]   ; rflags
    push rax
    popfq
    
    ret
