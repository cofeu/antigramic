#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <stdint.h>

typedef struct thread thread_t;

void sched_init(void);
void sched_start(void);
void sched_tick(void);
uint64_t sched_get_ticks(void);

#endif