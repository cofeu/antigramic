#ifndef _IPC_PORT_H
#define _IPC_PORT_H

#include <kernel.h>

#define PORT_MAX_MSG_SIZE 4096
#define PORT_NULL ((port_t)0)

typedef struct port *port_t;

struct port_message {
    size_t size;
    uint32_t flags;
    struct task *sender;
    uint8_t data[PORT_MAX_MSG_SIZE];
};

void ipc_init(void);
port_t port_create(const char *name, struct task *owner);
void port_destroy(port_t port);
int port_send(port_t port, const void *data, size_t size, uint32_t flags);
int port_recv(port_t port, void *buffer, size_t *size, uint32_t *flags);
port_t port_find(const char *name);

#endif
