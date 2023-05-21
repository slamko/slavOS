#ifndef INT_H
#define INT_H

#include <stdint.h>

enum IRQ_Ids {
    KBD_IRQ  = 1,
    COM2_IRQ = 3,
    COM1_IRQ = 4,
    SYSCALL  = 128
};

enum BASE_IRQ {
    KBD_MASK = (1 << KBD_IRQ),
    COM2_MASK = (1 << COM2_IRQ),
    COM1_MASK = (1 << COM1_IRQ),
};

struct idt_entry {
    uint16_t isr_low;
    uint16_t cs;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t isr_high;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct x86_cpu_state {
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;
} __attribute__((packed));

struct isr_stack {
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));

struct isr_full_stack {
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;
    uint32_t int_num;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));

void cli(void);

void init_idt();

/* void isr_x86(struct x86_cpu_state, uint32_t int_num, struct isr_stack);  */
void isr_x86 (struct isr_full_stack);

#endif
