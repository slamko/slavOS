#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdint.h>
#include <stdlib.h>

#define INIT_HEAP_SIZE (PAGE_SIZE * PT_SIZE)

void *alloc_test(size_t size);

void heap_init(uintptr_t heap_base);

void *kmalloc(size_t siz);

void kfree(void *addr);

#endif
