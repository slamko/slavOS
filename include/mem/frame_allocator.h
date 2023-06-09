#ifndef FRAME_ALLOC_H
#define FRAME_ALLOC_H

#include <stddef.h>
#include <stdint.h>
#include "mem/paging.h"

struct frame_map_addr {
    uint32_t frame_map;
    uint32_t frame;
};

int frame_alloc_init(size_t pmem_limit);

int alloc_frame(uintptr_t addr, uintptr_t *alloc_addr, unsigned int flags);

int find_alloc_frame(uintptr_t *alloc_addr, unsigned int flags);

int map_frame(page_table_t pt, unsigned int pte, uint16_t flags);

int alloc_nframes(size_t nframes, uintptr_t addr,
                        uintptr_t *alloc_addrs, uint16_t flags);

void dealloc_nframes(uintptr_t addr, size_t nframes);

void dealloc_frame(uintptr_t addr);

int find_alloc_nframes(size_t nframes, uintptr_t *alloc_addrs, uint16_t flags);

#endif
