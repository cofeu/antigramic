#include <sched/sched.h>
#include <mm/pmm.h>
#include <serial.h>
#include <string.h>

// String and external function declarations
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);
extern char *strncpy(char *dest, const char *src, size_t n);
extern void context_switch(struct task *prev, struct task *next);

#define MAX_TASKS 256
#define KERNEL_STACK_SIZE 8192
#define USER_STACK_SIZE 65536

static struct task *tasks[MAX_TASKS];
static size_t task_count = 0;
static struct task *current_task = NULL;
static struct task *idle_task = NULL;
static spinlock_t sched_lock = SPINLOCK_INIT;
static uint64_t next_pid = 1;

struct task *task_create(const char *name, int flags) {
    struct task *t = (struct task *)pmm_alloc();
    if (!t) return NULL;
    
    memset(t, 0, sizeof(struct task));
    t->pid = next_pid++;
    strncpy(t->name, name, sizeof(t->name) - 1);
    t->state = TASK_READY;
    t->flags = flags;
    t->priority = 10;
    t->time_slice = 10;
    
    t->kernel_stack = (uint64_t)pmm_alloc();
    if (!t->kernel_stack) {
        pmm_free((paddr_t)t);
        return NULL;
    }
    t->kernel_stack += KERNEL_STACK_SIZE;
    
    if (flags & TASK_USER) {
        t->user_stack = (uint64_t)pmm_alloc();
        if (!t->user_stack) {
            pmm_free((paddr_t)(t->kernel_stack - KERNEL_STACK_SIZE));
            pmm_free((paddr_t)t);
            return NULL;
        }
        t->user_stack += USER_STACK_SIZE;
    }
    
    spinlock_acquire(&sched_lock);
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i]) {
            tasks[i] = t;
            task_count++;
            break;
        }
    }
    spinlock_release(&sched_lock);
    
    return t;
}

void task_destroy(struct task *t) {
    if (!t) return;
    
    spinlock_acquire(&sched_lock);
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i] == t) {
            tasks[i] = NULL;
            task_count--;
            break;
        }
    }
    spinlock_release(&sched_lock);
    
    if (t->kernel_stack) {
        pmm_free((paddr_t)(t->kernel_stack - KERNEL_STACK_SIZE));
    }
    if (t->user_stack) {
        pmm_free((paddr_t)(t->user_stack - USER_STACK_SIZE));
    }
    pmm_free((paddr_t)t);
}

void sched_init(void) {
    idle_task = task_create("idle", TASK_KERNEL);
    if (idle_task) {
        idle_task->priority = 0;
        idle_task->state = TASK_READY;
    }
    serial_write_str("Scheduler: Initialized\n");
}

void sched_add(struct task *t) {
    if (!t) return;
    spinlock_acquire(&sched_lock);
    t->state = TASK_READY;
    spinlock_release(&sched_lock);
}

void sched_remove(struct task *t) {
    if (!t) return;
    spinlock_acquire(&sched_lock);
    t->state = TASK_EXITED;
    spinlock_release(&sched_lock);
}

struct task *sched_current(void) {
    return current_task;
}

void sched_yield(void) {
    spinlock_acquire(&sched_lock);
    
    if (!current_task) {
        current_task = idle_task;
        spinlock_release(&sched_lock);
        return;
    }
    
    struct task *next = current_task;
    size_t start = 0;
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i] == current_task) {
            start = i;
            break;
        }
    }
    
    for (size_t i = 1; i <= MAX_TASKS; i++) {
        size_t idx = (start + i) % MAX_TASKS;
        if (tasks[idx] && tasks[idx]->state == TASK_READY) {
            next = tasks[idx];
            break;
        }
    }
    
    if (next != current_task) {
        struct task *prev = current_task;
        current_task = next;
        spinlock_release(&sched_lock);
        context_switch(prev, next);
    } else {
        spinlock_release(&sched_lock);
    }
}

void sched_start(void) {
    current_task = idle_task;
    serial_write_str("Scheduler: Starting...\n");
    
    for (;;) {
        asm volatile("hlt");
        sched_yield();
    }
}

void sched_tick(void) {
    if (current_task && current_task != idle_task) {
        if (current_task->time_slice > 0) {
            current_task->time_slice--;
            if (current_task->time_slice == 0) {
                current_task->time_slice = 10;
                sched_yield();
            }
        }
    }
}
