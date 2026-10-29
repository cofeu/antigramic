#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define KERNEL_VERSION "0.1.0"

typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;

#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static inline void cpu_relax(void) {
    asm volatile("pause" ::: "memory");
}

static inline void barrier(void) {
    asm volatile("" ::: "memory");
}

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Spinlock
typedef struct {
    volatile uint64_t locked;
} spinlock_t;

#define SPINLOCK_INIT { 0 }

static inline void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
}

static inline void spinlock_acquire(spinlock_t *lock) {
    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        while (lock->locked) cpu_relax();
    }
}

static inline void spinlock_release(spinlock_t *lock) {
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}

// Task forward declaration
struct task;
typedef struct task *task_t;

#endif
