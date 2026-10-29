#include <arch.h>
#include <stdint.h>
#include <pmm.h>

#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4
#define PAGE_PWT 0x8
#define PAGE_PCD 0x10
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY 0x40
#define PAGE_HUGE 0x80
#define PAGE_GLOBAL 0x100

uint64_t *pml4 = (uint64_t*)0x100000;

void paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFF);
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        pdpt = (uint64_t*)pmm_alloc();
        pml4[pml4_idx] = (uint64_t)pdpt | flags | PAGE_PRESENT;
    }

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFF);
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        pd = (uint64_t*)pmm_alloc();
        pdpt[pdpt_idx] = (uint64_t)pd | flags | PAGE_PRESENT;
    }

    uint64_t *pt = (uint64_t*)(pd[pd_idx] & ~0xFFF);
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        pt = (uint64_t*)pmm_alloc();
        pd[pd_idx] = (uint64_t)pt | flags | PAGE_PRESENT;
    }

    pt[pt_idx] = phys | flags | PAGE_PRESENT;
    invlpg((void*)virt);
}

void paging_init(void) {
    for (uint64_t i = 0; i < 0x100000000; i += 0x1000) {
        paging_map_page(i, i, PAGE_WRITE);
    }
    write_cr3((uint64_t)pml4);
}