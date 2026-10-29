#ifndef _KERNEL_ARCH_X86_64_H
#define _KERNEL_ARCH_X86_64_H

#include <stdint.h>

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile ("wrmsr" :: "c"(msr), "a"((uint32_t)val), "d"(val >> 32));
}

static inline void invlpg(void *addr) {
    __asm__ volatile ("invlpg (%0)" :: "r"(addr) : "memory");
}

static inline uint64_t read_cr3(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void write_cr3(uint64_t val) {
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

#define MSR_IA32_APIC_BASE 0x1B
#define MSR_IA32_STAR 0xC0000081
#define MSR_IA32_LSTAR 0xC0000082
#define MSR_IA32_FMASK 0xC0000084

#define APIC_BASE_MSR 0xFEE00000

#endif