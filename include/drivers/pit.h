#ifndef PIT_H
#define PIT_H

#include "drivers/int.h"

int pit_init(unsigned long freq);

void sleep_us(uint32_t delay);

void sleep_ms(uint32_t delay);

void sleep_sec(uint32_t delay);

void pit_handler(struct isr_handler_args);
#endif
