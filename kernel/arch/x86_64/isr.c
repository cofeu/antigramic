#include <arch.h>
#include <stdint.h>
#include <apic.h>

void isr_handler(void *rsp) {
    uint64_t *regs = rsp;
    uint64_t int_num = regs[15];

    if (int_num == 0xE) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
    }

    if (int_num >= 32) {
        apic_eoi();
    }
}