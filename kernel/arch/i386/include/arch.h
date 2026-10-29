#ifndef _KERNEL_ARCH_I386_H
#define _KERNEL_ARCH_I386_H

#include <stdint.h>

static inline uint32_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return lo;
}

static inline void wrmsr(uint32_t msr, uint32_t val) {
    __asm__ volatile ("wrmsr" :: "c"(msr), "a"(val), "d"(0));
}

static inline void invlpg(void *addr) {
    __asm__ volatile ("invlpg [%0]" :: "r"(addr) : "memory");
}

static inline uint32_t read_cr3(void) {
    uint32_t val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void write_cr3(uint32_t val) {
    __asm__ volatile ("mov %0, %%cr3" :: "r"(val) : "memory");
}

static inline void enable_interrupts(void) {
    __asm__ volatile ("sti");
}

static inline void disable_interrupts(void) {
    __asm__ volatile ("cli");
}

static inline void hlt(void) {
    __asm__ volatile ("hlt");
}

#endif