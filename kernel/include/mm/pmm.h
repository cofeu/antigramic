#ifndef _MM_PMM_H
#define _MM_PMM_H

#include <kernel.h>

void pmm_init(void);
paddr_t pmm_alloc(void);
void pmm_free(paddr_t addr);
size_t pmm_get_free_pages(void);
size_t pmm_get_total_pages(void);

#endif
