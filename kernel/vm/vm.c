#include <vm.h>
#include <pmm.h>
#include <vmm.h>
#include <stdint.h>

static uint64_t mem_size;
static uint64_t kernel_end;

void vm_init(uint64_t ms, uint64_t ke) {
    mem_size = ms;
    kernel_end = ke;
    pmm_init(mem_size, kernel_end);
    vmm_init();
}

void *vm_alloc(size_t size) {
    size_t pages = (size + 4095) / 4096;
    void *base = vmm_alloc_page(VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE);
    if (!base) return NULL;

    for (size_t i = 1; i < pages; i++) {
        void *page = vmm_alloc_page(VMM_FLAGS_PRESENT | VMM_FLAGS_WRITE);
        if (!page) {
            for (size_t j = 0; j < i; j++) {
                vmm_free_page((char*)base + j * 4096);
            }
            return NULL;
        }
    }
    return base;
}

void vm_free(void *ptr, size_t size) {
    size_t pages = (size + 4095) / 4096;
    for (size_t i = 0; i < pages; i++) {
        vmm_free_page((char*)ptr + i * 4096);
    }
}

void *vm_map(void *virt, void *phys, size_t size, uint64_t flags) {
    size_t pages = (size + 4095) / 4096;
    for (size_t i = 0; i < pages; i++) {
        if (vmm_map((char*)virt + i * 4096, (char*)phys + i * 4096, flags) != 0) {
            for (size_t j = 0; j < i; j++) {
                vmm_unmap((char*)virt + j * 4096);
            }
            return NULL;
        }
    }
    return virt;
}

int vm_unmap(void *virt, size_t size) {
    size_t pages = (size + 4095) / 4096;
    for (size_t i = 0; i < pages; i++) {
        vmm_unmap((char*)virt + i * 4096);
    }
    return 0;
}