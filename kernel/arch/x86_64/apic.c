#include <arch.h>
#include <stdint.h>

#define APIC_BASE 0xFEE00000

#define APIC_ID         0x020
#define APIC_VERSION    0x030
#define APIC_TPR        0x080
#define APIC_APR        0x090
#define APIC_PPR        0x0A0
#define APIC_EOI        0x0B0
#define APIC_RRD        0x0C0
#define APIC_LDR        0x0D0
#define APIC_DFR        0x0E0
#define APIC_SVR        0x0F0
#define APIC_ISR        0x100
#define APIC_TMR        0x180
#define APIC_IRR        0x200
#define APIC_ESR        0x280
#define APIC_ICR_LOW    0x300
#define APIC_ICR_HIGH   0x310
#define APIC_LVT_TIMER  0x320
#define APIC_LVT_THERMAL 0x330
#define APIC_LVT_PMC    0x340
#define APIC_LVT_LINT0  0x350
#define APIC_LVT_LINT1  0x360
#define APIC_LVT_ERROR  0x370
#define APIC_TIMER_ICR  0x380
#define APIC_TIMER_CCR  0x390
#define APIC_TIMER_DCR  0x3E0

static volatile uint32_t *apic = (volatile uint32_t*)APIC_BASE;

static inline uint32_t apic_read(uint32_t reg) {
    return apic[reg / 16];
}

static inline void apic_write(uint32_t reg, uint32_t val) {
    apic[reg / 16] = val;
}

void apic_init(void) {
    uint64_t base = rdmsr(MSR_IA32_APIC_BASE);
    base &= ~0xFFF;
    base |= 0x800;
    wrmsr(MSR_IA32_APIC_BASE, base);

    apic_write(APIC_SVR, 0x1FF);
    apic_write(APIC_TPR, 0);
    apic_write(APIC_LVT_TIMER, 0x20000 | 0xFE);
    apic_write(APIC_TIMER_DCR, 0x3);
    apic_write(APIC_TIMER_ICR, 1000000);
}

void apic_eoi(void) {
    apic_write(APIC_EOI, 0);
}

void apic_timer_init(void) {
    apic_write(APIC_LVT_TIMER, 0x20000 | 0xFE);
    apic_write(APIC_TIMER_DCR, 0x3);
    apic_write(APIC_TIMER_ICR, 1000000);
}