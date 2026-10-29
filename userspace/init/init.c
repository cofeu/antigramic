#include <syscall.h>
#include <stdlib.h>
#include <string.h>

void init_process(void) {
    char *msg = "AntigramicOS Userspace Init\n";
    uint16_t *video = (uint16_t*)0xFFFF8000000B8000;
    for (int i = 0; msg[i]; i++) {
        video[i] = (uint16_t)msg[i] | 0x0F00;
    }

    while (1) {
        thread_yield();
    }
}