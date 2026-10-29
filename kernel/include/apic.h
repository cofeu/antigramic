#ifndef _KERNEL_APIC_H
#define _KERNEL_APIC_H

void apic_init(void);
void apic_eoi(void);
void apic_timer_init(void);

#endif