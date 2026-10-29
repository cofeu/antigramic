#ifndef _MM_VMM_H
#define _MM_VMM_H

#include <kernel.h>

void vmm_init(void);
int vmm_map(void *ctx, vaddr_t vaddr, paddr_t paddr, uint64_t flags);
void vmm_unmap(void *ctx, vaddr_t vaddr);
void *vmm_create_context(void);
void vmm_switch_context(void *ctx);
uint64_t *vmm_get_kernel_pml4(void);

#endif
