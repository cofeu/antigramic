#include <vmm.h>
#include <pmm.h>
#include <arch.h>
#include <stdint.h>
#include <string.h>

static uint64_t *kernel_pml4 = NULL;

uint64_t *vmm_get_pml4(void) { return kernel_pml4; }

void vmm_init(void) {
    kernel_pml4 = (uint64_t*)pmm_alloc();
    memset(kernel_pml4, 0, 4096);

    for (uint64_t i = 0; i < 0x100000000; i += 0x1000) {
        vmm_map((void*)i, (void*)i, VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE);
    }

    write_cr3((uint64_t)kernel_pml4);
}

static uint64_t *vmm_walk(uint64_t *pml4, uint64_t virt, int create) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFF);
    if (!(pml4[pml4_idx] & 1)) {
        if (!create) return NULL;
        pdpt = (uint64_t*)pmm_alloc();
        if (!pdpt) return NULL;
        memset(pdpt, 0, 4096);
        pml4[pml4_idx] = (uint64_t)pdpt | 3;
    }

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFF);
    if (!(pdpt[pdpt_idx] & 1)) {
        if (!create) return NULL;
        pd = (uint64_t*)pmm_alloc();
        if (!pd) return NULL;
        memset(pd, 0, 4096);
        pdpt[pdpt_idx] = (uint64_t)pd | 3;
    }

    uint64_t *pt = (uint64_t*)(pd[pd_idx] & ~0xFFF);
    if (!(pd[pd_idx] & 1)) {
        if (!create) return NULL;
        pt = (uint64_t*)pmm_alloc();
        if (!pt) return NULL;
        memset(pt, 0, 4096);
        pd[pd_idx] = (uint64_t)pt | 3;
    }

    return &pt[pt_idx];
}

int vmm_map(void *virt, void *phys, vmm_flags_t flags) {
    uint64_t *pte = vmm_walk(kernel_pml4, (uint64_t)virt, 1);
    if (!pte) return -1;
    *pte = ((uint64_t)phys & ~0xFFF) | flags;
    invlpg(virt);
    return 0;
}

int vmm_unmap(void *virt) {
    uint64_t *pte = vmm_walk(kernel_pml4, (uint64_t)virt, 0);
    if (!pte || !(*pte & 1)) return -1;
    *pte = 0;
    invlpg(virt);
    return 0;
}

void *vmm_alloc_page(vmm_flags_t flags) {
    void *phys = pmm_alloc();
    if (!phys) return NULL;

    for (uint64_t v = 0xFFFF800000000000; v < 0xFFFFFFFFFFFFF000; v += 4096) {
        if (!(*vmm_walk(kernel_pml4, v, 0) & 1)) {
            if (vmm_map((void*)v, phys, flags) == 0) {
                return (void*)v;
            }
        }
    }
    pmm_free(phys, 1);
    return NULL;
}

void vmm_free_page(void *addr) {
    void *phys = vmm_get_phys(addr);
    if (phys) {
        vmm_unmap(addr);
        pmm_free(phys, 1);
    }
}

void *vmm_get_phys(void *virt) {
    uint64_t *pte = vmm_walk(kernel_pml4, (uint64_t)virt, 0);
    if (!pte || !(*pte & 1)) return NULL;
    return (void*)(*pte & ~0xFFF);
}

void vmm_switch_context(void *pml4) {
    write_cr3((uint64_t)pml4);
}