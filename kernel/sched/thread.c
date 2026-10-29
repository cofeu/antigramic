#include <thread.h>
#include <vm.h>
#include <vmm.h>
#include <arch.h>
#include <stdint.h>
#include <string.h>

static thread_t threads[MAX_THREADS];
thread_t *current = NULL;
static thread_t *ready_queue = NULL;
static uint64_t next_tid = 1;

thread_t *sched_current(void) { return current; }

static void thread_trampoline(void) {
    thread_t *t = sched_current();
    ((void(*)())t->entry)();
    thread_exit();
}

thread_t *thread_create(void (*entry)(void), uint64_t stack_size, uint32_t priority) {
    if (stack_size < 4096) stack_size = 4096;
    stack_size = (stack_size + 4095) & ~4095;

    thread_t *t = NULL;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == THREAD_STATE_DEAD || threads[i].id == 0) {
            t = &threads[i];
            break;
        }
    }
    if (!t) return NULL;

    t->stack = (uint64_t)vm_alloc(stack_size);
    if (!t->stack) return NULL;
    t->stack_size = stack_size;

    t->pml4 = (uint64_t*)vm_alloc(4096);
    if (!t->pml4) {
        vm_free((void*)t->stack, stack_size);
        return NULL;
    }
    memcpy(t->pml4, vmm_get_pml4(), 4096);

    t->id = next_tid++;
    t->state = THREAD_STATE_READY;
    t->priority = priority;
    t->entry = (uint64_t)entry;
    t->rsp = t->stack + stack_size - 16;
    t->rbp = 0;
    t->rip = (uint64_t)thread_trampoline;
    t->rflags = 0x202;
    t->rbx = t->r12 = t->r13 = t->r14 = t->r15 = 0;
    t->next = ready_queue;
    ready_queue = t;

    return t;
}

void thread_destroy(thread_t *t) {
    if (t->stack) vm_free((void*)t->stack, t->stack_size);
    if (t->pml4) vm_free(t->pml4, 4096);
    t->state = THREAD_STATE_DEAD;
    t->id = 0;
}

extern void context_switch(thread_t *old, thread_t *new);

void thread_yield(void) {
    if (!current || !ready_queue) return;

    thread_t *next = ready_queue;
    ready_queue = ready_queue->next;
    next->next = NULL;

    thread_t *old = current;
    current->state = THREAD_STATE_READY;
    current->next = ready_queue;
    ready_queue = current;

    next->state = THREAD_STATE_RUNNING;
    current = next;

    context_switch(old, next);
}

void thread_exit(void) {
    disable_interrupts();
thread_t *dead = current;
    current->state = THREAD_STATE_DEAD;

    if (ready_queue) {
        current = ready_queue;
        ready_queue = ready_queue->next;
        current->next = NULL;
        current->state = THREAD_STATE_RUNNING;
        write_cr3((uint64_t)current->pml4);
        __asm__ volatile (
            "mov %0, %%rsp\n"
            "mov %1, %%rbp\n"
            "mov %2, %%rbx\n"
            "mov %3, %%r12\n"
            "mov %4, %%r13\n"
            "mov %5, %%r14\n"
            "mov %6, %%r15\n"
            :: "m"(current->rsp), "m"(current->rbp), "m"(current->rbx),
               "m"(current->r12), "m"(current->r13), "m"(current->r14), "m"(current->r15)
        );
    }

    thread_destroy(dead);
    enable_interrupts();
    while (1) hlt();
}

void sched_wakeup(thread_t *t) {
    if (t && t->state == THREAD_STATE_BLOCKED) {
        t->state = THREAD_STATE_READY;
        t->next = ready_queue;
        ready_queue = t;
    }
}

void sched_sleep(uint32_t timeout) {
    if (!current) return;
    current->state = THREAD_STATE_BLOCKED;
    thread_yield();
}