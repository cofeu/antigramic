#include <mouse.h>
#include <serial.h>

static int wait_input_clear(void) {
    for (uint32_t i=0;i<100000;i++) if (!(inb(0x64)&2)) return 1;
    return 0;
}
static int wait_output_full(void) {
    for (uint32_t i=0;i<100000;i++) if (inb(0x64)&1) return 1;
    return 0;
}
static int controller_command(uint8_t command) {
    if(!wait_input_clear())return 0;outb(0x64,command);return 1;
}
static int mouse_command(uint8_t command) {
    if(!controller_command(0xD4)||!wait_input_clear())return 0;
    outb(0x60,command);
    if(!wait_output_full())return 0;
    return inb(0x60)==0xFA;
}

void x86_ps2_mouse_init(void) {
    /* Drain stale bytes before enabling the auxiliary PS/2 port. */
    while(inb(0x64)&1) (void)inb(0x60);
    if(!controller_command(0xA8))return;
    if(!controller_command(0x20)||!wait_output_full())return;
    uint8_t config=inb(0x60);
    config=(uint8_t)((config|2)&~0x20); /* Enable IRQ12 and the auxiliary clock. */
    if(!controller_command(0x60)||!wait_input_clear())return;
    outb(0x60,config);
    if(!mouse_command(0xF6))return; /* Set standard 3-byte reporting defaults. */
    (void)mouse_command(0xF4);      /* Enable streaming. */
}

int x86_ps2_mouse_read(uint8_t *value) {
    if(!(inb(0x64)&1))return 0;
    uint8_t status=inb(0x64);
    if(!(status&0x20))return 0;
    *value=inb(0x60);
    return 1;
}
