#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <kernel.h>

uint64_t syscall_handler(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5);

#endif
