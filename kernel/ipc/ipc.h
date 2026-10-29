#ifndef _KERNEL_IPC_H
#define _KERNEL_IPC_H

#include <port.h>

int ipc_init(void);
int ipc_send(port_t port, message_t *msg, uint32_t timeout);
int ipc_receive(port_t port, message_t *msg, uint32_t timeout);
int ipc_rpc(port_t port, message_t *send, message_t *recv, uint32_t timeout);

#endif