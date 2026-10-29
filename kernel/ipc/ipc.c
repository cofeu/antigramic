#include <ipc.h>

int ipc_init(void) {
    return 0;
}

int ipc_send(port_t port, message_t *msg, uint32_t timeout) {
    return msg_send(port, msg, timeout);
}

int ipc_receive(port_t port, message_t *msg, uint32_t timeout) {
    return msg_receive(port, msg, timeout);
}

int ipc_rpc(port_t port, message_t *send, message_t *recv, uint32_t timeout) {
    return msg_rpc(port, send, recv, timeout);
}