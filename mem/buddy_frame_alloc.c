#include "lib/slibc.h"
#include "mem/buddy_alloc.h"
#include "drivers/fb.h"
#include "mem/paging.h"
#include "mem/allocator.h"
#include "mem/frame_allocator.h"
#include "kernel/error.h"
#include "lib/kernel.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_ORDER           11
#define MAX_BUDDY_SIZE      0x400000 
#define MAP_SIZE            32
#define MAX_BUDDY_NFRAMES   1024

typedef uint8_t order_t;

struct free_list {
    struct free_list *next;
    uintptr_t addr;
};

struct free_area {
    struct free_list free_list;
    size_t num_free;
};

struct free_area free_area[MAX_ORDER];

uint32_t *buddy_maps[MAX_ORDER];

static inline uint8_t get_buddy_order(size_t nframes) {
    if (nframes > MAX_BUDDY_NFRAMES) {
        return MAX_ORDER - 1;
    }
    
    for (uint8_t order = 0; order < MAX_ORDER; order++) {
        if (nframes <= (1u << order)) {
            return order;
        }
    }

    return MAX_ORDER - 1;
}

static inline bool is_frame_free(uint32_t bitmap, uint8_t frame) {
    return !(bitmap & (1 << frame));
}

static int insert_buddy(struct free_list **fl, uintptr_t addr) {
    struct free_list *old_head = (*fl)->next;
    (*fl)->next = kmalloc(sizeof(**fl));
    if (!(*fl)->next) {
        return ENOMEM;
    }
    
    (*fl)->next->next = old_head;
    (*fl)->next->addr = addr;
    return 0;
}

static inline void remove_free_head(order_t order) {    
    struct free_area *f_area = &free_area[order];
    if (f_area->num_free == 0) {
        return;
    }
    
    struct free_list *new_next = f_area->free_list.next->next;

    /* fb_print_hex((uintptr_t)f_area->free_list.next); */
    kfree(f_area->free_list.next);
    f_area->free_list.next = new_next;
    f_area->num_free --;
}
static inline void set_addrs(uintptr_t *addrs, uintptr_t base,
                             size_t size, uint16_t flags) {
    for (unsigned int i = 0; i < size; i++) {
        addrs[i] = (base + (i * PAGE_SIZE)) | flags;
    }
}

static inline struct frame_map_addr addr_to_map_id(order_t order,
                                                   uintptr_t addr) {
    struct frame_map_addr map;
    map.frame_map = addr / (PAGE_SIZE * MAP_SIZE * (1 << order));
    map.frame =
        (addr - (map.frame_map * (PAGE_SIZE * MAP_SIZE * (1 << order)))) /
        ((1 << order) * PAGE_SIZE);

    return map;
}

static inline uintptr_t map_to_addr(order_t order,
                                    struct frame_map_addr map) {
    uintptr_t base = (map.frame_map * MAP_SIZE * PAGE_SIZE * (1 << order));
    uintptr_t frame_addr = map.frame * (1 << order);
    return base + frame_addr;
}

static inline void set_frame_unused(order_t order, uintptr_t addr) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);
    buddy_maps[order][map_id.frame_map] &= ~(1 << map_id.frame);
}

static inline void set_frame_used(order_t order, uintptr_t addr) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);
    buddy_maps[order][map_id.frame_map] |= (1 << map_id.frame);
}

static inline void set_map_used(order_t order, uintptr_t addr,
                             size_t nframes) {
    struct frame_map_addr map_id = addr_to_map_id(order, addr);

    for (unsigned int i = 0; i < nframes; i++) {
        buddy_maps[order][map_id.frame_map] |= (1 << (map_id.frame + i));
    }
}

static uintptr_t buddy_slice(uintptr_t addr,
                       order_t start, order_t target) {
    set_frame_used(start, addr);
    
    free_area[start - 1].num_free++;
    struct free_list *next_order_fl = &free_area[start - 1].free_list;
    uintptr_t sec_buddy_addr = addr +
                ((PAGE_SIZE * (1 << start)) / 2);

    insert_buddy(&next_order_fl, sec_buddy_addr);
    debug_log("sec buddy addr");
    fb_print_hex((uintptr_t)sec_buddy_addr);
   
    if (start - 1 > target) {
        /* klog("slice\n"); */
        return buddy_slice(addr, start - 1, target);
    }
    
    /* klog("sec bodyy"); */
    /* fb_print_hex(sec_buddy_addr); */
    return addr;
}

int buddy_alloc_frames(uintptr_t *addrs, size_t nframes, uint16_t flags) {
    if (!addrs) {
        return EINVAL;
    }

    order_t order = get_buddy_order(nframes);
    struct free_list *free = free_area[order].free_list.next;
    debug_log("order");
    fb_print_num(order);
    /* klog("num free"); */
    /* fb_print_num(free_area[order].num_free); */
    

    if (free_area[order].num_free > 0) {
        debug_log("free frame available");
        remove_free_head(order);
        set_addrs(addrs, free->addr, nframes, flags);
        set_frame_used(order, free->addr);
        return 0;
    }

    for (unsigned int i = order + 1; i < MAX_ORDER; i++) {
        struct free_list *upper_fl = free_area[i].free_list.next;
        debug_log("search");
        struct free_area *fa = &free_area[i];
        if (fa->num_free > 0) {
            /* klog("free"); */
            remove_free_head(i);
            uintptr_t addr = buddy_slice(upper_fl->addr, i, order);
            set_addrs(addrs, addr, nframes, flags);
            return 0;
        }
    }

    return 0;
}

int buddy_alloc_at_addr(uintptr_t base, uintptr_t *addrs, size_t nframes,
                        uint16_t flags) {
    if (!addrs) {
        return EINVAL;
    }

    order_t order = get_buddy_order(nframes);
    /* klog("order"); */
    /* fb_print_num(order); */
    /* fb_print_num((uintptr_t)free_area[order].free_list.next); */

    if (free_area[order].num_free > 0) {
        struct free_list *free = NULL;
        
        struct free_list *init_fl = &free_area[order].free_list;
        struct free_list **prev_fl = &init_fl;
        for (struct free_list **fl = &free_area[order].free_list.next;
             fl; fl = &(*fl)->next) {
            /* klog("addr"); */
            /* fb_print_hex((*fl)->addr); */
            if ((*fl)->addr == base) {
                free = *fl;
                break;
            }
            prev_fl = fl;
        }

        if (!free) {
            return EINVAL;
        }

        free_area[order].num_free--;
        (*prev_fl)->next = free->next;
        /* klog("Alloc at addr"); */
        
        set_addrs(addrs, free->addr, nframes, flags);
        set_frame_used(order, free->addr);
        return 0;
    }
 
    return EINVAL;
}

int buddy_alloc_frame(uintptr_t *addr, uint16_t flags) {
    return buddy_alloc_frames(addr, 1, flags);
}

void buddy_coalesce(order_t order, uintptr_t addr) {
    struct frame_map_addr map = addr_to_map_id(order, addr);
    uint8_t buddy_frame = map.frame + 1;

    if (map.frame % 2) {
        buddy_frame = map.frame - 1;
    }

    set_frame_unused(order, addr);
    if (order + 1 >= MAX_ORDER) {
        return;
    }

    if (is_frame_free(buddy_maps[order][map.frame_map], buddy_frame)) {
        
        buddy_coalesce(order + 1, addr);
    }

    struct frame_map_addr buddy_map = map;
    buddy_map.frame = buddy_frame;
    struct free_list *fl = &free_area[order].free_list;
    insert_buddy(&fl, map_to_addr(order, buddy_map));
}

void buddy_free_frames_rec(order_t order, uintptr_t addr) {
    struct frame_map_addr map = addr_to_map_id(order, addr);

    if (!is_frame_free(buddy_maps[order][map.frame_map], map.frame)) {
        buddy_coalesce(order, addr);
        klog("exit free");
        return;
    }
    
    buddy_free_frames_rec(order + 1, addr);
}

void buddy_free_frames(uintptr_t addr, size_t nframes) {
    order_t order = get_buddy_order(nframes);
    buddy_free_frames_rec(order, addr);
}

void buddy_free_frame(uintptr_t addr) {
    buddy_free_frames(addr, 1);
}

int buddy_alloc_init(size_t mem_limit) {
    if (!mem_limit) {
        return EINVAL;
    }

    memset(&free_area, 0, sizeof(free_area));
    for (unsigned int i = 0; i < MAX_ORDER; i++) {
        size_t map_size = mem_limit / (PAGE_SIZE * MAP_SIZE * (1 << i));
        buddy_maps[i] = kmalloc(map_size);

        if (!buddy_maps[i]) {
            return ENOMEM;
        }
        /* klog("num free"); */
        /* fb_print_num(free_area[i].num_free); */
    }

    size_t max_map_size = mem_limit / (MAX_BUDDY_SIZE);
    struct free_list **cur_free = &(free_area[MAX_ORDER - 1].free_list.next);
    free_area[MAX_ORDER - 1].num_free = max_map_size;

    for (unsigned int i = 0; i < max_map_size; i++) {
        *cur_free = kmalloc(sizeof(**cur_free));

        if (!*cur_free) {
            return ENOMEM;
        }

        (*cur_free)->addr = (i * MAX_BUDDY_SIZE);
        /* fb_print_hex((uintptr_t)cur_free); */
        cur_free = &(*cur_free)->next;
    }
    
    return 0;
}

uintptr_t addrs[1024];

void buddy_test(size_t mem) {
    buddy_alloc_init(mem);
    memset(addrs, 0, sizeof(addrs));
    int ret;

    memset(addrs, 0, sizeof(addrs));
    klog("Buddy find alloc\n");
    ret = buddy_alloc_frames(addrs, 4, R_W|PRESENT);

    for (unsigned int i = 0; i < 4; i ++) {
        fb_print_hex(addrs[i]);
    }
    fb_print_num(ret);

    buddy_free_frames(addrs[0], 4);

    memset(addrs, 0, sizeof(addrs));
    klog("Buddy find alloc\n");
    ret = buddy_alloc_frames(addrs, 2, R_W|PRESENT);

    for (unsigned int i = 0; i < 2; i ++) {
        fb_print_hex(addrs[i]);
    }
    fb_print_num(ret);

    return;
 
}