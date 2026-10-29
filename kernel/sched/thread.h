#ifndef _KERNEL_SCHED_THREAD_H
#define _KERNEL_SCHED_THREAD_H

#include <stdint.h>
#include <vm.h>

#define THREAD_STATE_READY  0
#define THREAD_STATE_RUNNING 1
#define THREAD_STATE_BLOCKED 2
#define THREAD_STATE_DEAD   3

#define MAX_THREADS 256

typedef struct thread {
    uint64_t id;
    uint32_t state;
    uint32_t priority;
    uint64_t entry;
    uint64_t stack;
    uint64_t stack_size;
    uint64_t *pml4;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rflags;
    uint64_t rbx, r12, r13, r14, r15;
    struct thread *next;
} thread_t;

thread_t *thread_create(void (*entry)(void), uint64_t stack_size, uint32_t priority);
void thread_destroy(thread_t *t);
void thread_yield(void);
void thread_exit(void);
thread_t *sched_current(void);
extern thread_t *current;
void sched_wakeup(thread_t *t);
void sched_sleep(uint32_t timeout);

#endif