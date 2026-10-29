#include <arch.h>
#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct tss {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

static struct gdt_entry gdt[7];
static struct tss tss;

extern void gdt_flush(uint64_t);
extern void tss_flush(void);

void gdt_set_gate(int num, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_init(void) {
    struct gdt_ptr gp;
    gp.limit = sizeof(gdt) - 1;
    gp.base = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xA0);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xA0);
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xA0);
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xA0);

    uint64_t tss_base = (uint64_t)&tss;
    uint64_t tss_limit = sizeof(tss);
    gdt[5].base_low = tss_base & 0xFFFF;
    gdt[5].base_mid = (tss_base >> 16) & 0xFF;
    gdt[5].base_high = (tss_base >> 24) & 0xFF;
    gdt[5].limit_low = tss_limit & 0xFFFF;
    gdt[5].granularity = (tss_limit >> 16) & 0x0F;
    gdt[5].granularity |= 0x00;
    gdt[5].access = 0x89;

    gdt[6].base_low = (tss_base >> 32) & 0xFFFF;
    gdt[6].base_mid = (tss_base >> 48) & 0xFFFF;
    gdt[6].limit_low = 0;
    gdt[6].granularity = 0;
    gdt[6].access = 0;

    tss.rsp[0] = 0x100000;
    tss.ist[0] = 0x200000;

    gdt_flush((uint64_t)&gp);
    tss_flush();
}