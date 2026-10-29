#ifndef _KERNEL_VM_H
#define _KERNEL_VM_H

#include <stdint.h>
#include <stddef.h>

void vm_init(uint64_t mem_size, uint64_t kernel_end);
void *vm_alloc(size_t size);
void vm_free(void *ptr, size_t size);
void *vm_map(void *virt, void *phys, size_t size, uint64_t flags);
int vm_unmap(void *virt, size_t size);

#endif