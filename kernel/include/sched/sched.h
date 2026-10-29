#ifndef _SCHED_SCHED_H
#define _SCHED_SCHED_H

#include <kernel.h>

#define TASK_KERNEL 0
#define TASK_USER   1

#define TASK_READY  0
#define TASK_RUNNING 1
#define TASK_BLOCKED 2
#define TASK_EXITED  3

struct task {
    uint64_t pid;
    char name[32];
    int state;
    int flags;
    int priority;
    int time_slice;
    
    uint64_t kernel_stack;
    uint64_t user_stack;
    
    // Register context for context switching
    uint64_t rsp, rbp, rbx, r12, r13, r14, r15;
    uint64_t rip;
    uint64_t rflags;
    
    // Page table (for user tasks)
    void *pml4;
};

struct task *task_create(const char *name, int flags);
void task_destroy(struct task *t);
void sched_init(void);
void sched_add(struct task *t);
void sched_remove(struct task *t);
struct task *sched_current(void);
void sched_yield(void);
void sched_start(void);
void sched_tick(void);

#endif
