#ifndef _KERNEL_IPC_H
#define _KERNEL_IPC_H

#include <stdint.h>

#define PORT_RIGHT_SEND    0x1
#define PORT_RIGHT_RECEIVE 0x2
#define PORT_RIGHT_PORTSET 0x4

typedef uint32_t port_t;
typedef uint32_t port_right_t;

struct message {
    uint32_t header;
    uint32_t body[63];
    port_t reply_port;
};
typedef struct message message_t;

int ipc_init(void);
port_t port_create(void);
int port_destroy(port_t port);
int msg_send(port_t port, message_t *msg, uint32_t timeout);
int msg_receive(port_t port, message_t *msg, uint32_t timeout);
int msg_rpc(port_t port, message_t *send, message_t *recv, uint32_t timeout);

#endif