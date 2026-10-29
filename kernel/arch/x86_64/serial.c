#include <string.h>
#include <serial.h>

#define COM1 0x3F8

static int serial_initialized = 0;

void serial_init(void) {
    if (serial_initialized) return;
    
    outb(COM1 + 1, 0x00);    // Disable interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB
    outb(COM1 + 0, 0x03);    // Divisor low (38400 baud)
    outb(COM1 + 1, 0x00);    // Divisor high
    outb(COM1 + 3, 0x03);    // 8N1, disable DLAB
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    
    serial_initialized = 1;
}

void serial_putc(char c) {
    if (!serial_initialized) serial_init();
    
    while ((inb(COM1 + 5) & 0x20) == 0);
    outb(COM1, c);
}

void serial_write(const char *s, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n') serial_putc('\r');
        serial_putc(s[i]);
    }
}

void serial_write_str(const char *s) {
    serial_write(s, strlen(s));
}

int serial_read(char *c) {
    if (!serial_initialized) serial_init();
    
    if (inb(COM1 + 5) & 0x01) {
        *c = inb(COM1);
        return 1;
    }
    return 0;
}
