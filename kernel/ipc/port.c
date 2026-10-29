#include <ipc/port.h>
#include <mm/pmm.h>
#include <sched/sched.h>
#include <serial.h>
#include <string.h>

// String function declarations for freestanding
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dest, const void *src, size_t n);
extern char *strncpy(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);

#define PORT_MAGIC 0x504F5254
#define MAX_PORTS 1024
#define PORT_QUEUE_SIZE 64

struct port {
    uint32_t magic;
    int refcount;
    char name[32];
    spinlock_t lock;
    struct port_message *queue[PORT_QUEUE_SIZE];
    size_t head, tail, count;
    struct task *owner;
    struct port *next;
};

static struct port *ports[MAX_PORTS];
static spinlock_t ports_lock = SPINLOCK_INIT;
static size_t port_count = 0;

void ipc_init(void) {
    memset(ports, 0, sizeof(ports));
    port_count = 0;
    serial_write_str("IPC: Port system initialized\n");
}

static struct port *port_alloc(const char *name, struct task *owner) {
    struct port *p = (struct port *)pmm_alloc();
    if (!p) return NULL;
    
    memset(p, 0, sizeof(struct port));
    p->magic = PORT_MAGIC;
    p->refcount = 1;
    strncpy(p->name, name, sizeof(p->name) - 1);
    spinlock_init(&p->lock);
    p->owner = owner;
    
    spinlock_acquire(&ports_lock);
    for (size_t i = 0; i < MAX_PORTS; i++) {
        if (!ports[i]) {
            ports[i] = p;
            port_count++;
            spinlock_release(&ports_lock);
            return p;
        }
    }
    spinlock_release(&ports_lock);
    pmm_free((paddr_t)p);
    return NULL;
}

port_t port_create(const char *name, struct task *owner) {
    struct port *p = port_alloc(name, owner);
    return p ? (port_t)p : PORT_NULL;
}

void port_destroy(port_t port) {
    if (!port) return;
    struct port *p = (struct port *)port;
    
    if (p->magic != PORT_MAGIC) return;
    
    spinlock_acquire(&p->lock);
    if (--p->refcount > 0) {
        spinlock_release(&p->lock);
        return;
    }
    
    while (p->count > 0) {
        struct port_message *msg = p->queue[p->head];
        p->head = (p->head + 1) % PORT_QUEUE_SIZE;
        p->count--;
        pmm_free((paddr_t)msg);
    }
    spinlock_release(&p->lock);
    
    spinlock_acquire(&ports_lock);
    for (size_t i = 0; i < MAX_PORTS; i++) {
        if (ports[i] == p) {
            ports[i] = NULL;
            port_count--;
            break;
        }
    }
    spinlock_release(&ports_lock);
    
    pmm_free((paddr_t)p);
}

int port_send(port_t port, const void *data, size_t size, uint32_t flags) {
    if (!port || size > PORT_MAX_MSG_SIZE) return -1;
    struct port *p = (struct port *)port;
    
    if (p->magic != PORT_MAGIC) return -1;
    
    struct port_message *msg = (struct port_message *)pmm_alloc();
    if (!msg) return -1;
    
    msg->size = size;
    msg->flags = flags;
    msg->sender = sched_current();
    memcpy(msg->data, data, size);
    
    spinlock_acquire(&p->lock);
    if (p->count >= PORT_QUEUE_SIZE) {
        spinlock_release(&p->lock);
        pmm_free((paddr_t)msg);
        return -1;
    }
    
    p->queue[p->tail] = msg;
    p->tail = (p->tail + 1) % PORT_QUEUE_SIZE;
    p->count++;
    spinlock_release(&p->lock);
    
    return 0;
}

int port_recv(port_t port, void *buffer, size_t *size, uint32_t *flags) {
    if (!port || !buffer || !size) return -1;
    struct port *p = (struct port *)port;
    
    if (p->magic != PORT_MAGIC) return -1;
    
    spinlock_acquire(&p->lock);
    if (p->count == 0) {
        spinlock_release(&p->lock);
        return -1;
    }
    
    struct port_message *msg = p->queue[p->head];
    p->head = (p->head + 1) % PORT_QUEUE_SIZE;
    p->count--;
    spinlock_release(&p->lock);
    
    if (*size < msg->size) {
        pmm_free((paddr_t)msg);
        return -1;
    }
    
    memcpy(buffer, msg->data, msg->size);
    *size = msg->size;
    if (flags) *flags = msg->flags;
    
    pmm_free((paddr_t)msg);
    return 0;
}

port_t port_find(const char *name) {
    spinlock_acquire(&ports_lock);
    for (size_t i = 0; i < MAX_PORTS; i++) {
        if (ports[i] && strcmp(ports[i]->name, name) == 0) {
            spinlock_release(&ports_lock);
            return (port_t)ports[i];
        }
    }
    spinlock_release(&ports_lock);
    return PORT_NULL;
}
