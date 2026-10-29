#include <pmm.h>
#include <stdint.h>
#include <string.h>

static uint64_t *bitmap = NULL;
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t bitmap_size = 0;

void pmm_init(uint64_t mem_size, uint64_t kernel_end) {
    total_pages = mem_size / 4096;
    bitmap_size = (total_pages + 63) / 64;
    bitmap = (uint64_t*)kernel_end;
    memset(bitmap, 0xFF, bitmap_size * 8);

    uint64_t kernel_pages = (kernel_end + 4095) / 4096;
    for (uint64_t i = 0; i < kernel_pages; i++) {
        bitmap[i / 64] &= ~(1ULL << (i % 64));
        free_pages++;
    }

    for (uint64_t i = kernel_pages; i < total_pages; i++) {
        if (!(bitmap[i / 64] & (1ULL << (i % 64)))) {
            free_pages++;
        }
    }
}

void *pmm_alloc(void) {
    for (uint64_t i = 0; i < bitmap_size; i++) {
        if (bitmap[i] != 0xFFFFFFFFFFFFFFFFULL) {
            for (int b = 0; b < 64; b++) {
                if (!(bitmap[i] & (1ULL << b))) {
                    uint64_t page = i * 64 + b;
                    if (page >= total_pages) return NULL;
                    bitmap[i] |= (1ULL << b);
                    free_pages--;
                    return (void*)(page * 4096);
                }
            }
        }
    }
    return NULL;
}

void pmm_free(void *addr, size_t pages) {
    uint64_t page = (uint64_t)addr / 4096;
    for (size_t i = 0; i < pages; i++) {
        if (page + i < total_pages) {
            bitmap[(page + i) / 64] &= ~(1ULL << ((page + i) % 64));
            free_pages++;
        }
    }
}

uint64_t pmm_get_free(void) { return free_pages * 4096; }
uint64_t pmm_get_used(void) { return (total_pages - free_pages) * 4096; }