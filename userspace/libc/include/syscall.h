#ifndef _USER_SYSCALL_H
#define _USER_SYSCALL_H

#include <stdint.h>

#define SYS_YIELD       0
#define SYS_THREAD_CREATE 1
#define SYS_THREAD_EXIT   2
#define SYS_PORT_CREATE   3
#define SYS_PORT_DESTROY  4
#define SYS_MSG_SEND      5
#define SYS_MSG_RECEIVE   6
#define SYS_VM_ALLOC      7
#define SYS_VM_FREE       8
#define SYS_VM_MAP        9
#define SYS_GET_TICKS     10

static inline int64_t syscall0(int num) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t syscall1(int num, uint64_t a1) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t syscall2(int num, uint64_t a1, uint64_t a2) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t syscall3(int num, uint64_t a1, uint64_t a2, uint64_t a3) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t syscall4(int num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(a4)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t syscall5(int num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(a4), "r"(a5)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t syscall6(int num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(a4), "r"(a5), "r"(a6)
        : "rcx", "r11", "memory"
    );
    return ret;
}

#define thread_yield() syscall0(SYS_YIELD)
#define thread_create(entry, stack, prio) syscall3(SYS_THREAD_CREATE, (uint64_t)entry, stack, prio)
#define thread_exit() syscall0(SYS_THREAD_EXIT)
#define port_create() syscall0(SYS_PORT_CREATE)
#define port_destroy(port) syscall1(SYS_PORT_DESTROY, port)
#define msg_send(port, msg, timeout) syscall3(SYS_MSG_SEND, port, (uint64_t)msg, timeout)
#define msg_receive(port, msg, timeout) syscall3(SYS_MSG_RECEIVE, port, (uint64_t)msg, timeout)
#define vm_alloc(size) syscall1(SYS_VM_ALLOC, size)
#define vm_free(ptr, size) syscall2(SYS_VM_FREE, (uint64_t)ptr, size)
#define vm_map(virt, phys, size, flags) syscall4(SYS_VM_MAP, (uint64_t)virt, (uint64_t)phys, size, flags)
#define get_ticks() syscall0(SYS_GET_TICKS)

#endif