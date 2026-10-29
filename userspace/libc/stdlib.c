#include <stdlib.h>
#include <string.h>

extern char _heap_start;
extern char _heap_end;

static char *heap_ptr = &_heap_start;

void *malloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 7) & ~7;
    if (heap_ptr + size > &_heap_end) return NULL;
    void *ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}

void free(void *ptr) {
    (void)ptr;
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    void *new_ptr = malloc(size);
    if (new_ptr && ptr) {
        memcpy(new_ptr, ptr, size);
    }
    return new_ptr;
}

void abort(void) {
    while (1) __asm__("hlt");
}

void exit(int status) {
    (void)status;
    while (1) __asm__("hlt");
}

int abs(int n) { return n < 0 ? -n : n; }
long labs(long n) { return n < 0 ? -n : n; }
long long llabs(long long n) { return n < 0 ? -n : n; }