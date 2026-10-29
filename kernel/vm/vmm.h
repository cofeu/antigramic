#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H

#include <stdint.h>
#include <stddef.h>

#define VMM_FLAGS_PRESENT  0x1
#define VMM_FLAGS_WRITE    0x2
#define VMM_FLAGS_USER     0x4
#define VMM_FLAGS_NX       0x8000000000000000ULL

typedef uint64_t vmm_flags_t;

void vmm_init(void);
void *vmm_alloc_page(vmm_flags_t flags);
void vmm_free_page(void *addr);
int vmm_map(void *virt, void *phys, vmm_flags_t flags);
int vmm_unmap(void *virt);
void *vmm_get_phys(void *virt);
void vmm_switch_context(void *pml4);
uint64_t *vmm_get_pml4(void);

#endif