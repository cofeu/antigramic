#include <mm/vmm.h>
#include <mm/pmm.h>
#include <serial.h>
#include <string.h>

#define PTE_PRESENT  (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_USER     (1ULL << 2)
#define PTE_PWT      (1ULL << 3)
#define PTE_PCD      (1ULL << 4)
#define PTE_ACCESSED (1ULL << 5)
#define PTE_DIRTY    (1ULL << 6)
#define PTE_HUGE     (1ULL << 7)
#define PTE_GLOBAL   (1ULL << 8)
#define PTE_NX       (1ULL << 63)

static uint64_t *kernel_pml4 = NULL;

static uint64_t *vmm_get_table(uint64_t *table, size_t idx, int create) {
    uint64_t entry = table[idx];
    if (entry & PTE_PRESENT) {
        return (uint64_t *)(entry & PAGE_MASK);
    }
    if (!create) return NULL;
    
    paddr_t page = pmm_alloc();
    if (!page) return NULL;
    
    memset((void *)page, 0, PAGE_SIZE);
    table[idx] = page | PTE_PRESENT | PTE_WRITABLE;
    return (uint64_t *)page;
}

void vmm_init(void) {
    serial_write_str("VMM: Allocating PML4...\n");
    kernel_pml4 = (uint64_t *)pmm_alloc();
    if (!kernel_pml4) {
        serial_write_str("VMM: Failed to allocate PML4\n");
        return;
    }
    memset(kernel_pml4, 0, PAGE_SIZE);
    serial_write_str("VMM: PML4 allocated\n");
    
    serial_write_str("VMM: Mapping kernel...\n");
    for (paddr_t p = 0; p < 0x100000000ULL; p += 0x200000) {
        vmm_map(NULL, p, p, PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL | PTE_HUGE);
    }
    
    serial_write_str("VMM: Loading CR3...\n");
    asm volatile("mov %0, %%cr3" : : "r"(kernel_pml4));
    
    serial_write_str("VMM: Initialized with kernel PML4\n");
}

int vmm_map(void *ctx, vaddr_t vaddr, paddr_t paddr, uint64_t flags) {
    uint64_t *pml4 = ctx ? (uint64_t *)ctx : kernel_pml4;
    
    if (flags & PTE_HUGE) {
        size_t pml4_idx = (vaddr >> 39) & 0x1FF;
        size_t pdpt_idx = (vaddr >> 30) & 0x1FF;
        size_t pd_idx   = (vaddr >> 21) & 0x1FF;
        
        uint64_t *pdpt = vmm_get_table(pml4, pml4_idx, 1);
        if (!pdpt) return -1;
        
        uint64_t *pd = vmm_get_table(pdpt, pdpt_idx, 1);
        if (!pd) return -1;
        
        pd[pd_idx] = (paddr & PAGE_MASK) | flags;
        return 0;
    }
    
    size_t pml4_idx = (vaddr >> 39) & 0x1FF;
    size_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    size_t pd_idx   = (vaddr >> 21) & 0x1FF;
    size_t pt_idx   = (vaddr >> 12) & 0x1FF;
    
    uint64_t *pdpt = vmm_get_table(pml4, pml4_idx, 1);
    if (!pdpt) return -1;
    
    uint64_t *pd = vmm_get_table(pdpt, pdpt_idx, 1);
    if (!pd) return -1;
    
    uint64_t *pt = vmm_get_table(pd, pd_idx, 1);
    if (!pt) return -1;
    
    pt[pt_idx] = (paddr & PAGE_MASK) | flags;
    return 0;
}

void vmm_unmap(void *ctx, vaddr_t vaddr) {
    uint64_t *pml4 = ctx ? (uint64_t *)ctx : kernel_pml4;
    
    size_t pml4_idx = (vaddr >> 39) & 0x1FF;
    size_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    size_t pd_idx   = (vaddr >> 21) & 0x1FF;
    size_t pt_idx   = (vaddr >> 12) & 0x1FF;
    
    uint64_t *pdpt = vmm_get_table(pml4, pml4_idx, 0);
    if (!pdpt) return;
    
    uint64_t *pd = vmm_get_table(pdpt, pdpt_idx, 0);
    if (!pd) return;
    
    uint64_t *pt = vmm_get_table(pd, pd_idx, 0);
    if (!pt) return;
    
    pt[pt_idx] = 0;
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

void *vmm_create_context(void) {
    uint64_t *pml4 = (uint64_t *)pmm_alloc();
    if (!pml4) return NULL;
    memset(pml4, 0, PAGE_SIZE);
    
    for (size_t i = 256; i < 512; i++) {
        pml4[i] = kernel_pml4[i];
    }
    return pml4;
}

void vmm_switch_context(void *ctx) {
    if (ctx) {
        asm volatile("mov %0, %%cr3" : : "r"(ctx));
    } else {
        asm volatile("mov %0, %%cr3" : : "r"(kernel_pml4));
    }
}

uint64_t *vmm_get_kernel_pml4(void) {
    return kernel_pml4;
}
