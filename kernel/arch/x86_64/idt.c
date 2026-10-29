#include <arch.h>
#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

extern void idt_flush(uint64_t);
extern void *isr_stub_table[];

void idt_set_gate(int n, uint64_t handler, uint16_t sel, uint8_t flags) {
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].offset_mid = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].selector = sel;
    idt[n].ist = 0;
    idt[n].type_attr = flags;
    idt[n].zero = 0;
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }

    idt_flush((uint64_t)&idtp);
}