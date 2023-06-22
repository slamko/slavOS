#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdlib.h>
#include "drivers/int.h"

#define PT_SIZE 1024
#define PAGE_SIZE 4096

#define to_uintptr(ptr) ((uintptr_t)(void *)(ptr))

enum {
    PRESENT              = (1 << 0),
    R_W                  = (1 << 1),
    USER                 = (1 << 2),
    PWT                  = (1 << 3),
    PCD                  = (1 << 4),
    ACCESSED             = (1 << 5),
    DIRTY                = (1 << 6),
    PS                   = (1 << 7),
};

typedef uintptr_t * page_table_t;
typedef uintptr_t ** page_dir_t;

uintptr_t to_phys_addr(void *virt_addr);

void paging_init(void);

void page_fault(struct isr_handler_args args);

uintptr_t *clone_page_dir(uintptr_t *pd);

uintptr_t *clone_page_table(uintptr_t *pt);

extern void *vga_buf;

#endif
