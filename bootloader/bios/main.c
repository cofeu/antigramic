#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t addr;
    uint32_t len;
    uint32_t type;
} __attribute__((packed)) mmap_entry_t;

void kmain(void) {
    uint16_t* video = (uint16_t*)0xB8000;
    const char* msg = "AntigramicOS Kernel Basliyor...";
    for (int i = 0; msg[i]; i++) {
        video[i] = (uint16_t)msg[i] | 0x0F00;
    }

    while (1) __asm__("hlt");
}