#include <mm/pmm.h>
#include <sched/sched.h>
#include <ipc/port.h>
#include <syscall.h>
#include <sched/sched.h>
#include <ipc/port.h>
#include <mm/vmm.h>

uint64_t syscall_handler(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    switch (num) {
        case 0:  // exit
            task_destroy(sched_current());
            sched_yield();
            return 0;
        case 1:  // write
            return 0;
        case 2:  // read
            return 0;
        case 3:  // yield
            sched_yield();
            return 0;
        default:
            return -1;
    }
}

// Stubs for missing functions
void *vm_alloc(size_t size) { return (void *)pmm_alloc(); }
void vm_free(void *ptr) { pmm_free((paddr_t)ptr); }
struct task *thread_create(const char *name, void (*entry)(void*), void *arg) {
    return task_create(name, TASK_USER);
}
int msg_send(port_t port, const void *data, size_t size) {
    return port_send(port, data, size, 0);
}
int msg_receive(port_t port, void *data, size_t *size) {
    return port_recv(port, data, size, NULL);
}
uint64_t sched_get_ticks(void) { return 0; }
