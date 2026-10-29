#ifndef KERNEL_X86_MOUSE_H
#define KERNEL_X86_MOUSE_H
#include <stdint.h>
void x86_ps2_mouse_init(void);
int x86_ps2_mouse_read(uint8_t *value);
#endif
