#ifndef _KERNEL_PMM_H
#define _KERNEL_PMM_H

#include <stdint.h>
#include <stddef.h>

void pmm_init(uint64_t mem_size, uint64_t kernel_end);
void *pmm_alloc(void);
void pmm_free(void *addr, size_t pages);
uint64_t pmm_get_free(void);
uint64_t pmm_get_used(void);

#endif