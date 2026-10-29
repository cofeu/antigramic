#include <string.h>
#include <mm/pmm.h>
#include <serial.h>
#include <vm.h>

#define PMM_MAX_PAGES (128 * 1024 * 1024 / PAGE_SIZE)
static uint8_t pmm_bitmap[PMM_MAX_PAGES / 8];
static size_t pmm_total_pages = 0;
static size_t pmm_free_pages = 0;
static paddr_t pmm_memory_base = 0;
static paddr_t pmm_memory_limit = 0;
extern char kernel_end[];

static inline void pmm_set_page(paddr_t addr) {
    size_t idx = (addr - pmm_memory_base) / PAGE_SIZE;
    pmm_bitmap[idx / 8] |= (1 << (idx % 8));
}

static inline void pmm_clear_page(paddr_t addr) {
    size_t idx = (addr - pmm_memory_base) / PAGE_SIZE;
    pmm_bitmap[idx / 8] &= ~(1 << (idx % 8));
}

static inline int pmm_test_page(paddr_t addr) {
    size_t idx = (addr - pmm_memory_base) / PAGE_SIZE;
    return (pmm_bitmap[idx / 8] & (1 << (idx % 8))) != 0;
}

void pmm_init(void) {
    // Avoid all loaded kernel sections and static data buffers.
    pmm_memory_base = 0x100000;
    pmm_memory_limit = 0x8000000;
    pmm_total_pages = (pmm_memory_limit - pmm_memory_base) / PAGE_SIZE;
    
    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    pmm_free_pages = 0;
    
    paddr_t reserved_end = ((paddr_t)kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (reserved_end < 0x200000) reserved_end = 0x200000;
    if (reserved_end > pmm_memory_limit) reserved_end = pmm_memory_limit;
    for (paddr_t p = pmm_memory_base; p < pmm_memory_limit; p += PAGE_SIZE) {
        if (p < reserved_end) pmm_set_page(p);
        else { pmm_clear_page(p); pmm_free_pages++; }
    }
    
    serial_write_str("PMM: Initialized, ");
    char buf[32];
    size_t n = pmm_free_pages;
    char *p = buf + 31;
    *p = 0;
    do {
        *--p = "0123456789"[n % 10];
        n /= 10;
    } while (n);
    serial_write_str(p);
    serial_write_str(" pages free\n");
}

paddr_t pmm_alloc(void) {
    for (size_t i = 0; i < pmm_total_pages; i++) {
        if (!(pmm_bitmap[i / 8] & (1 << (i % 8)))) {
            paddr_t addr = pmm_memory_base + i * PAGE_SIZE;
            pmm_set_page(addr);
            pmm_free_pages--;
            return addr;
        }
    }
    return 0;
}

void pmm_free(paddr_t addr) {
    if (addr < pmm_memory_base || addr >= pmm_memory_limit) return;
    if (!pmm_test_page(addr)) return;
    pmm_clear_page(addr);
    pmm_free_pages++;
}

size_t pmm_get_free_pages(void) {
    return pmm_free_pages;
}

size_t pmm_get_total_pages(void) {
    return pmm_total_pages;
}
