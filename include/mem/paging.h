#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>
#include <stdint.h>
#include "drivers/int.h"

#define PT_SIZE 1024
#define PAGE_SIZE 4096

#define to_uintptr(ptr) ((uintptr_t)(void *)(ptr))

#define align_up(num, alignment) (((num) - ((num) % (alignment))) + \
    ((((num) % (alignment))) ? (alignment) : 0))

#define page_align_up(num) align_up(num, PAGE_SIZE) 

#define align_down(num, alignment) ((num) - ((num) % (alignment)))

#define page_align_down(num) align_down((num), PAGE_SIZE)
#define div_align_up(num, alignment) (((num) / (alignment)) + \
    (((num) % (alignment)) ? 1 : 0))

#define div_align_down(num, alignment) ((num) / (alignment))

#define div_page_align_up(num) div_align_up(num, PAGE_SIZE)

#define div_page_align_down(num) div_align_down(num, PAGE_SIZE) 


typedef uint16_t pte_t;

enum PAGING_STRUCT_FLAGS {
    PRESENT              = (1 << 0),
    R_W                  = (1 << 1),
    USER                 = (1 << 2),
    PWT                  = (1 << 3),
    PCD                  = (1 << 4),
    ACCESSED             = (1 << 5),
    DIRTY                = (1 << 6),
    PS                   = (1 << 7),
    GLOBAL               = (1 << 8),
};

typedef uintptr_t * page_table_t;

struct page_dir {
    uintptr_t *page_tables;

    page_table_t *page_tables_virt;

    uintptr_t pd_phys_addr;
};

void paging_init(size_t pmem_amount);

static inline uintptr_t get_virt_addr(uint16_t pde, uint16_t pte) {
    return (pde * 0x400000) + (pte * PAGE_SIZE);
}

static inline int tab_present(uintptr_t descriptor) {
    return descriptor & PRESENT;
}

static inline uint16_t get_tab_flags(uintptr_t table) {
    return (table & 0xfff);
}

static inline uintptr_t get_tab_pure_addr(uintptr_t table) {
    return (table & ~0xfff);
}

void print_cr0(void);

void flush_tlb(void);

void flush_page(uintptr_t virt_addr);

int clone_cur_page_dir(struct page_dir *new_pd);

int switch_page_dir(struct page_dir *pd);

int map_alloc_pt(struct page_dir *pd, page_table_t *pt, uint16_t pde,
    uint16_t flags);

int get_pde_pte(uintptr_t addr, uint16_t *pde_p, uint16_t *pte_p);

uintptr_t get_tab_pure_addr(uintptr_t table);

void unmap_page(struct page_dir *pd, pte_t pde, pte_t pte);

extern struct page_dir *cur_pd;

extern struct page_dir *kernel_pd;

void page_fault(struct isr_handler_args *args);

void flush_pages(uintptr_t *virt_addr, size_t npages);

void flush_pages_contiguous(uintptr_t virt_addr, size_t npages);

uintptr_t to_phys_addr(const struct page_dir *pd, uintptr_t virt_addr);

uintptr_t ptr_to_phys_addr(const struct page_dir *pd, void *ptr);

void free_pd(struct page_dir *pd);

int clone_page_dir(struct page_dir *restrict pd,
                   struct page_dir *restrict new_pd, int i);

int clone_page_table(struct page_dir *pd, pte_t pde,
page_table_t *new_pt_ptr, uintptr_t *new_pt_phys_addr);

extern uintptr_t map_limit;

int map_alloc_npt(struct page_dir *pd, page_table_t *pt, size_t npt,
                  pte_t pde, uint16_t flags);

void unmap_pages(struct page_dir *pd, pte_t pde, pte_t pte, size_t npages);

#endif
